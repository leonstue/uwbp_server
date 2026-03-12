#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <sdbus-c++/sdbus-c++.h>

// ---- Signal-Handling ----

static volatile sig_atomic_t g_running = 1;

extern "C" void handleSignal(int)
{
    g_running = 0;
}

// ---- NM-Konstanten (standalone, kein Link gegen net-Modul) ----

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
        // Deaktivieren
        try
        {
            nm->callMethod("DeactivateConnection")
                .onInterface(NM_IFACE)
                .withArguments(sdbus::ObjectPath{e.activeConnectionPath});
            std::cerr << "[watchdog] deactivated " << e.activeConnectionPath << "\n";
        }
        catch (const sdbus::Error& ex)
        {
            std::cerr << "[watchdog] deactivate failed: " << ex.what() << "\n";
        }

        // Settings loeschen
        try
        {
            auto cp = sdbus::createProxy(*conn,
                                          sdbus::ServiceName{NM_SERVICE},
                                          sdbus::ObjectPath{e.connectionPath});
            cp->callMethod("Delete").onInterface(NM_SETTINGS_CONN_IFACE);
            std::cerr << "[watchdog] deleted " << e.connectionPath << "\n";
        }
        catch (const sdbus::Error& ex)
        {
            std::cerr << "[watchdog] delete failed: " << ex.what() << "\n";
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
        std::cerr << "Usage: uwbp_watchdog <server_pid> <state_file_path>\n";
        return 1;
    }

    pid_t serverPid           = static_cast<pid_t>(std::stol(argv[1]));
    std::string stateFilePath = argv[2];

    // Signal-Handler installieren
    struct sigaction sa{};
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    std::cerr << "[watchdog] monitoring PID " << serverPid
              << ", state file: " << stateFilePath << "\n";

    // Poll-Loop: alle 2 Sekunden pruefen
    while (g_running)
    {
        if (!processAlive(serverPid))
        {
            std::cerr << "[watchdog] server PID " << serverPid
                      << " is gone. Cleaning up...\n";

            auto entries = readStateFile(stateFilePath);
            if (!entries.empty())
            {
                cleanupConnections(entries);
                std::filesystem::remove(stateFilePath);
            }

            std::cerr << "[watchdog] cleanup complete. Exiting.\n";
            return 0;
        }

        // 2s Sleep in kleinen Schritten (fuer schnelle Signal-Reaktion)
        for (int i = 0; i < 20 && g_running; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "[watchdog] received signal, shutting down.\n";
    return 0;
}
