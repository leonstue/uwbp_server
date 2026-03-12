#include "../header/NetworkManagerClient.h"
#include "../header/ApConfig.h"

#include <sdbus-c++/sdbus-c++.h>

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>

namespace uwbp::net
{
    static constexpr const char *NM_SERVICE = "org.freedesktop.NetworkManager";
    static constexpr const char *NM_PATH = "/org/freedesktop/NetworkManager";
    static constexpr const char *NM_IFACE = "org.freedesktop.NetworkManager";
    static constexpr const char *NM_DEV_IFACE = "org.freedesktop.NetworkManager.Device";
    static constexpr const char *NM_SETTINGS_CONN_IFACE =
        "org.freedesktop.NetworkManager.Settings.Connection";

    using NmSettingsMap = std::map<std::string,
                                   std::map<std::string, sdbus::Variant>>;

    struct NetworkManagerClient::Impl
    {
        std::unique_ptr<sdbus::IConnection> connection;
        std::unique_ptr<sdbus::IProxy> nm;
        std::vector<NmActiveAp> activeAps;

        Impl()
        {
            // System-Bus (NetworkManager ist System-Dienst)
            connection = sdbus::createSystemBusConnection();
            nm = sdbus::createProxy(*connection,
                                    sdbus::ServiceName{NM_SERVICE},
                                    sdbus::ObjectPath{NM_PATH});
            // sdbus-c++ v2: kein finishRegistration() noetig
        }

        std::uint32_t getState() const
        {
            // Property "State" auf org.freedesktop.NetworkManager
            return nm->getProperty("State").onInterface(NM_IFACE).get<std::uint32_t>();
        }

        std::vector<sdbus::ObjectPath> getDevices() const
        {
            std::vector<sdbus::ObjectPath> devices;
            nm->callMethod("GetDevices")
                .onInterface(NM_IFACE)
                .storeResultsTo(devices);
            return devices;
        }

        NmDeviceInfo readDevice(const sdbus::ObjectPath &devPath) const
        {
            auto dev = sdbus::createProxy(*connection,
                                          sdbus::ServiceName{NM_SERVICE},
                                          devPath);
            NmDeviceInfo info;
            info.objectPath = std::string(devPath);

            info.iface = dev->getProperty("Interface").onInterface(NM_DEV_IFACE).get<std::string>();
            info.deviceType = dev->getProperty("DeviceType").onInterface(NM_DEV_IFACE).get<std::uint32_t>();
            info.state = dev->getProperty("State").onInterface(NM_DEV_IFACE).get<std::uint32_t>();

            return info;
        }

        sdbus::ObjectPath findDevicePath(const std::string& iface) const
        {
            for (const auto& dp : getDevices())
            {
                auto info = readDevice(dp);
                if (info.iface == iface)
                    return dp;
            }
            throw std::runtime_error("WiFi device not found: " + iface);
        }

        NmActiveAp createAp(const ApConfig& cfg)
        {
            auto devicePath = findDevicePath(cfg.iface);

            // Connection-Settings aufbauen: a{sa{sv}}
            NmSettingsMap settings;

            // "connection" Sektion
            settings["connection"]["type"]        = sdbus::Variant{std::string{"802-11-wireless"}};
            settings["connection"]["autoconnect"] = sdbus::Variant{false};
            settings["connection"]["id"]          = sdbus::Variant{std::string{"uwbp-ap-" + cfg.ssid}};

            // "802-11-wireless" Sektion
            // SSID muss als ay (vector<uint8_t>) gesendet werden
            std::vector<std::uint8_t> ssidBytes(cfg.ssid.begin(), cfg.ssid.end());
            settings["802-11-wireless"]["ssid"]    = sdbus::Variant{ssidBytes};
            settings["802-11-wireless"]["mode"]    = sdbus::Variant{std::string{"ap"}};
            settings["802-11-wireless"]["band"]    = sdbus::Variant{cfg.band};
            settings["802-11-wireless"]["channel"] = sdbus::Variant{cfg.channel};
            settings["802-11-wireless"]["hidden"]  = sdbus::Variant{cfg.hidden};

            // "802-11-wireless-security" Sektion
            settings["802-11-wireless-security"]["key-mgmt"] =
                sdbus::Variant{std::string{"wpa-psk"}};
            settings["802-11-wireless-security"]["psk"] =
                sdbus::Variant{cfg.psk};

            // "ipv4" Sektion -- "shared" aktiviert dnsmasq DHCP + NAT
            settings["ipv4"]["method"] = sdbus::Variant{std::string{"shared"}};

            // Options: persist=volatile (nicht auf Disk gespeichert)
            std::map<std::string, sdbus::Variant> options;
            options["persist"] = sdbus::Variant{std::string{"volatile"}};

            // AddAndActivateConnection2 aufrufen
            sdbus::ObjectPath connPath;
            sdbus::ObjectPath activeConnPath;
            std::map<std::string, sdbus::Variant> result;

            nm->callMethod("AddAndActivateConnection2")
                .onInterface(NM_IFACE)
                .withArguments(settings,
                               devicePath,
                               sdbus::ObjectPath{"/"},
                               options)
                .storeResultsTo(connPath, activeConnPath, result);

            NmActiveAp ap;
            ap.connectionPath       = std::string(connPath);
            ap.activeConnectionPath = std::string(activeConnPath);
            activeAps.push_back(ap);
            return ap;
        }

        void deactivateConnection(const std::string& activeConnPath)
        {
            try
            {
                nm->callMethod("DeactivateConnection")
                    .onInterface(NM_IFACE)
                    .withArguments(sdbus::ObjectPath{activeConnPath});
            }
            catch (const sdbus::Error&)
            {
                // Connection evtl. bereits weg -- ignorieren
            }
        }

        void deleteConnectionSettings(const std::string& connPath)
        {
            try
            {
                auto connProxy = sdbus::createProxy(*connection,
                                                     sdbus::ServiceName{NM_SERVICE},
                                                     sdbus::ObjectPath{connPath});
                connProxy->callMethod("Delete")
                    .onInterface(NM_SETTINGS_CONN_IFACE);
            }
            catch (const sdbus::Error&)
            {
                // Settings-Objekt evtl. bereits weg -- ignorieren
            }
        }

        void removeAp(const NmActiveAp& ap)
        {
            deactivateConnection(ap.activeConnectionPath);
            deleteConnectionSettings(ap.connectionPath);
            std::erase_if(activeAps, [&](const NmActiveAp& a) {
                return a.activeConnectionPath == ap.activeConnectionPath;
            });
        }

        void removeAllAps()
        {
            auto copy = activeAps;
            for (const auto& ap : copy)
                removeAp(ap);
        }
    };

    // --- Public API ---

    NetworkManagerClient::NetworkManagerClient()
        : impl_(new Impl())
    {
    }

    NetworkManagerClient::~NetworkManagerClient()
    {
        delete impl_;
    }

    std::uint32_t NetworkManagerClient::getState() const
    {
        return impl_->getState();
    }

    std::vector<NmDeviceInfo> NetworkManagerClient::listDevices() const
    {
        std::vector<NmDeviceInfo> out;
        for (const auto &p : impl_->getDevices())
            out.push_back(impl_->readDevice(p));
        return out;
    }

    bool NetworkManagerClient::tryGetDeviceByIface(const std::string &iface, NmDeviceInfo &out) const
    {
        for (const auto &d : listDevices())
        {
            if (d.iface == iface)
            {
                out = d;
                return true;
            }
        }
        return false;
    }

    NmActiveAp NetworkManagerClient::createAp(const ApConfig& cfg)
    {
        return impl_->createAp(cfg);
    }

    void NetworkManagerClient::removeAp(const NmActiveAp& ap)
    {
        impl_->removeAp(ap);
    }

    void NetworkManagerClient::removeAllAps()
    {
        impl_->removeAllAps();
    }

    const std::vector<NmActiveAp>& NetworkManagerClient::activeAps() const
    {
        return impl_->activeAps;
    }

} // namespace uwbp::net
