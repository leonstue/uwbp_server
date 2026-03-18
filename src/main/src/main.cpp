#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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
#include "Logger.h"
#include "HttpServer.h"
#include "RestRouter.h"
#include "RestController.hpp"
#include "DeviceManager.hpp"

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

// resolve path relative to our own binary
static std::filesystem::path getExeDir()
{
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
}

static pid_t spawnWatchdog(const std::string& stateFilePath,
                           const std::string& logDir)
{
    std::string wdPath = (getExeDir() / "uwbp_watchdog").string();
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
        const_cast<char*>(logDir.c_str()),
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

    // log dir next to the binary
    std::string logDir = (getExeDir() / "logs").string();
    uwbp::common::Logger log(logDir + "/server.log");

    try
    {
        uwbp::net::NetworkManagerClient nmc;
        uwbp::common::StateFile stateFile(STATE_FILE_PATH);

        // tell dnsmasq to resolve "uwbp" to our AP ip.
        // has to be written before the AP starts so dnsmasq picks it up.
        {
            std::filesystem::create_directories("/etc/NetworkManager/dnsmasq-shared.d");
            std::ofstream dns("/etc/NetworkManager/dnsmasq-shared.d/uwbp.conf",
                              std::ios::trunc);
            dns << "address=/uwbp/10.42.0.1\n";
        }
        log.info("DNS alias 'uwbp' -> 10.42.0.1 configured");

        // single AP for both ESPs and the user frontend
        uwbp::net::ApConfig apCfg;
        apCfg.ssid    = "UWBP";
        apCfg.psk     = "abcd1234"; // TODO: load from config file
        apCfg.iface   = "wlan0";
        apCfg.band    = "bg";
        apCfg.channel = 6;
        apCfg.hidden  = false;

        log.info("Creating AP '" + apCfg.ssid + "'...");
        auto ap = nmc.createAp(apCfg);
        log.info("  active: " + ap.activeConnectionPath);

        // persist state so watchdog can cleanup if we crash
        writeState(stateFile, nmc);

        pid_t wdPid = spawnWatchdog(stateFile.path(), logDir);
        log.info("Watchdog spawned (PID " + std::to_string(wdPid) + ")");

        // ---- http server ----
        auto deviceManager = std::make_shared<uwbp::uwb::DeviceManager>();

        uwbp::server::RestRouter router;
        uwbp::server::registerRoutes(router, &g_running, deviceManager);

        uwbp::server::HttpServer httpServer(router, 8080);
        httpServer.start();
        log.info("HTTP server listening on port 8080");

        log.info("uwbp_server running. Ctrl+C to stop.");
        while (g_running)
            pause();

        // ---- shutdown ----
        log.info("Shutting down...");
        httpServer.stop();
        nmc.removeAllAps();
        stateFile.remove();

        if (wdPid > 0)
        {
            kill(wdPid, SIGTERM);
            waitpid(wdPid, nullptr, 0);
        }

        log.info("Goodbye.");
    }
    catch (const std::exception& ex)
    {
        log.error(std::string("Fatal: ") + ex.what());
        return 1;
    }

    return 0;
}
