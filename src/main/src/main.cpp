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
// Produktion: "/run/uwbp/state" (braucht /run/uwbp Verzeichnis)

static void writeState(const uwbp::common::StateFile& sf,
                       const uwbp::net::NetworkManagerClient& nmc)
{
    std::vector<std::pair<std::string, std::string>> entries;
    for (const auto& ap : nmc.activeAps())
        entries.emplace_back(ap.connectionPath, ap.activeConnectionPath);
    sf.write(entries);
}

// Watchdog-Binary liegt im selben Verzeichnis wie uwbp_server
static std::string getWatchdogPath()
{
    // /proc/self/exe -> absoluter Pfad der laufenden Binary
    auto selfPath = std::filesystem::read_symlink("/proc/self/exe");
    return (selfPath.parent_path() / "uwbp_watchdog").string();
}

static pid_t spawnWatchdog(const std::string& stateFilePath)
{
    std::string wdPath = getWatchdogPath();
    std::string pidStr = std::to_string(getpid());

    // posix_spawn statt fork(): erbt keine D-Bus File-Descriptors,
    // verhindert Probleme wenn exec fehlschlaegt.
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
    // Signal-Handler
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    try
    {
        uwbp::net::NetworkManagerClient nmc;
        uwbp::common::StateFile stateFile(STATE_FILE_PATH);

        // Ein AP fuer ESPs und User-Frontend gemeinsam
        uwbp::net::ApConfig apCfg;
        apCfg.ssid    = "UWBP";
        apCfg.psk     = "uwbp-secret-psk"; // TODO: aus Config-File laden
        apCfg.iface   = "wlan0";
        apCfg.band    = "bg";
        apCfg.channel = 6;
        apCfg.hidden  = false;

        std::cout << "Creating AP '" << apCfg.ssid << "'...\n";
        auto ap = nmc.createAp(apCfg);
        std::cout << "  active: " << ap.activeConnectionPath << "\n";

        // State-File schreiben (fuer Watchdog-Cleanup bei Crash)
        writeState(stateFile, nmc);

        // Watchdog spawnen
        pid_t wdPid = spawnWatchdog(stateFile.path());
        std::cout << "Watchdog spawned (PID " << wdPid << ")\n";

        // ---- Server-Loop ----
        std::cout << "uwbp_server running. Ctrl+C to stop.\n";
        while (g_running)
        {
            // TODO: Poco HTTP Server Event-Loop kommt hier hin
            pause(); // Auf Signal warten
        }

        // ---- Sauberes Shutdown ----
        std::cout << "\nShutting down...\n";
        nmc.removeAllAps();
        stateFile.remove();

        // Watchdog beenden
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
