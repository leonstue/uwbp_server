#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <sdbus-c++/sdbus-c++.h>

// ---- signal handling ----

static volatile sig_atomic_t g_running = 1;

extern "C" void handleSignal(int)
{
    g_running = 0;
}

// ---- simple file logger (standalone, doesnt link against common module) ----

static std::ofstream g_logFile;

static void logMsg(const std::string& level, const std::string& msg)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << " [" << level << "] " << msg;

    std::cerr << "[watchdog] " << msg << "\n";

    if (g_logFile.is_open())
    {
        g_logFile << ss.str() << "\n";
        g_logFile.flush();
    }
}

// ---- NM constants (standalone, doesnt link against the net module) ----

static constexpr const char* NM_SERVICE = "org.freedesktop.NetworkManager";
static constexpr const char* NM_PATH    = "/org/freedesktop/NetworkManager";
static constexpr const char* NM_IFACE   = "org.freedesktop.NetworkManager";
static constexpr const char* NM_SETTINGS_CONN_IFACE =
    "org.freedesktop.NetworkManager.Settings.Connection";

struct ApEntry
{
    std::string connectionPath;
    std::string activeConnectionPath;
};

static std::vector<ApEntry> readStateFile(const std::string& path)
{
    std::vector<ApEntry> out;
    std::ifstream ifs(path);
    std::string line;
    while (std::getline(ifs, line))
    {
        auto tab = line.find('\t');
        if (tab != std::string::npos)
            out.push_back({line.substr(0, tab), line.substr(tab + 1)});
    }
    return out;
}

static void cleanupConnections(const std::vector<ApEntry>& entries)
{
    auto conn = sdbus::createSystemBusConnection();
    auto nm   = sdbus::createProxy(*conn,
                                    sdbus::ServiceName{NM_SERVICE},
                                    sdbus::ObjectPath{NM_PATH});
    for (const auto& e : entries)
    {
        // deactivate
        try
        {
            nm->callMethod("DeactivateConnection")
                .onInterface(NM_IFACE)
                .withArguments(sdbus::ObjectPath{e.activeConnectionPath});
            logMsg("INFO", "deactivated " + e.activeConnectionPath);
        }
        catch (const sdbus::Error& ex)
        {
            logMsg("ERROR", std::string("deactivate failed: ") + ex.what());
        }

        // delete settings
        try
        {
            auto cp = sdbus::createProxy(*conn,
                                          sdbus::ServiceName{NM_SERVICE},
                                          sdbus::ObjectPath{e.connectionPath});
            cp->callMethod("Delete").onInterface(NM_SETTINGS_CONN_IFACE);
            logMsg("INFO", "deleted " + e.connectionPath);
        }
        catch (const sdbus::Error& ex)
        {
            logMsg("ERROR", std::string("delete failed: ") + ex.what());
        }
    }
}

static bool processAlive(pid_t pid)
{
    return (kill(pid, 0) == 0);
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: uwbp_watchdog <server_pid> <state_file_path> [log_dir]\n";
        return 1;
    }

    pid_t serverPid           = static_cast<pid_t>(std::stol(argv[1]));
    std::string stateFilePath = argv[2];

    // open log file if log dir was passed
    if (argc >= 4)
    {
        std::string logDir = argv[3];
        std::filesystem::create_directories(logDir);
        g_logFile.open(logDir + "/watchdog.log", std::ios::app);
    }

    struct sigaction sa{};
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    logMsg("INFO", "monitoring PID " + std::to_string(serverPid)
                   + ", state file: " + stateFilePath);

    // poll every ~2s, check if the server is still alive
    while (g_running)
    {
        if (!processAlive(serverPid))
        {
            logMsg("INFO", "server PID " + std::to_string(serverPid)
                           + " is gone. Cleaning up...");

            auto entries = readStateFile(stateFilePath);
            if (!entries.empty())
            {
                cleanupConnections(entries);
                std::filesystem::remove(stateFilePath);
            }

            logMsg("INFO", "cleanup complete. Exiting.");
            return 0;
        }

        // sleep in small chunks so we react to signals quickly
        for (int i = 0; i < 20 && g_running; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    logMsg("INFO", "received signal, shutting down.");
    return 0;
}
