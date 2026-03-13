#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

#include "NetworkManagerClient.h"
#include "ApConfig.h"
#include "StateFile.h"

static volatile sig_atomic_t g_running = 1;

extern "C" void onSignal(int) { g_running = 0; }

static constexpr const char* STATE_FILE_PATH = "/tmp/uwbp_state";

static void writeState(const uwbp::common::StateFile& sf,
                       const uwbp::net::NetworkManagerClient& nmc)
{
    std::vector<std::pair<std::string, std::string>> entries;
    for (const auto& ap : nmc.activeAps())
        entries.emplace_back(ap.connectionPath, ap.activeConnectionPath);
    sf.write(entries);
}

// resolve watchdog binary path relative to our own executable
static std::string getWatchdogPath()
{
    auto selfPath = std::filesystem::read_symlink("/proc/self/exe");
    return (selfPath.parent_path() / "uwbp_watchdog").string();
}

static pid_t spawnWatchdog(const std::string& stateFilePath)
{
    std::string wdPath = getWatchdogPath();
    std::string pidStr = std::to_string(getpid());

    // using posix_spawn instead of fork() here because fork duplicates
    // the dbus file descriptors which messes with NetworkManager
    // when the child process exits
    posix_spawn_file_actions_t fileActions;
    posix_spawn_file_actions_init(&fileActions);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    char* argv[] = {
        const_cast<char*>("uwbp_watchdog"),
        const_cast<char*>(pidStr.c_str()),
        const_cast<char*>(stateFilePath.c_str()),
        nullptr
    };

    pid_t child = -1;
    int err = posix_spawn(&child, wdPath.c_str(), &fileActions, &attr, argv, environ);

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&fileActions);

    if (err != 0)
    {
        std::cerr << "Failed to spawn watchdog: " << wdPath
                  << " (" << strerror(err) << ")\n";
        return -1;
    }
    return child;
}

int main()
{
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    try
    {
        uwbp::net::NetworkManagerClient nmc;
        uwbp::common::StateFile stateFile(STATE_FILE_PATH);

        // single AP for both ESPs and the user frontend
        uwbp::net::ApConfig apCfg;
        apCfg.ssid    = "UWBP";
        apCfg.psk     = "uwbp-secret-psk"; // TODO: load from config file
        apCfg.iface   = "wlan0";
        apCfg.band    = "bg";
        apCfg.channel = 6;
        apCfg.hidden  = false;

        std::cout << "Creating AP '" << apCfg.ssid << "'...\n";
        auto ap = nmc.createAp(apCfg);
        std::cout << "  active: " << ap.activeConnectionPath << "\n";

        // persist state so watchdog can cleanup if we crash
        writeState(stateFile, nmc);

        pid_t wdPid = spawnWatchdog(stateFile.path());
        std::cout << "Watchdog spawned (PID " << wdPid << ")\n";

        // ---- server loop ----
        std::cout << "uwbp_server running. Ctrl+C to stop.\n";
        while (g_running)
        {
            // TODO: poco http server goes here
            pause();
        }

        // ---- shutdown ----
        std::cout << "\nShutting down...\n";
        nmc.removeAllAps();
        stateFile.remove();

        if (wdPid > 0)
        {
            kill(wdPid, SIGTERM);
            waitpid(wdPid, nullptr, 0);
        }

        std::cout << "Goodbye.\n";
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
