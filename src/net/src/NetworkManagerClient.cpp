#include "../header/NetworkManagerClient.h"

#include <sdbus-c++/sdbus-c++.h>

#include <memory>
#include <stdexcept>

namespace uwbp::net
{
    static constexpr const char *NM_SERVICE = "org.freedesktop.NetworkManager";
    static constexpr const char *NM_PATH = "/org/freedesktop/NetworkManager";
    static constexpr const char *NM_IFACE = "org.freedesktop.NetworkManager";
    static constexpr const char *NM_DEV_IFACE = "org.freedesktop.NetworkManager.Device";

    struct NetworkManagerClient::Impl
    {
        std::unique_ptr<sdbus::IConnection> connection;
        std::unique_ptr<sdbus::IProxy> nm;

        Impl()
        {
            // System-Bus (NetworkManager ist System-Dienst)
            connection = sdbus::createSystemBusConnection();
            nm = sdbus::createProxy(*connection,
                                    sdbus::ServiceName{NM_SERVICE},
                                    sdbus::ObjectPath{NM_PATH});
            nm->finishRegistration();
        }

        std::uint32_t getState() const
        {
            // Property "State" auf org.freedesktop.NetworkManager
            return nm->getProperty("State").onInterface(NM_IFACE);
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
            dev->finishRegistration();

            NmDeviceInfo info;
            info.objectPath = std::string(devPath);

            info.iface = dev->getProperty("Interface").onInterface(NM_DEV_IFACE);
            info.deviceType = dev->getProperty("DeviceType").onInterface(NM_DEV_IFACE);
            info.state = dev->getProperty("State").onInterface(NM_DEV_IFACE);

            return info;
        }
    };

    NetworkManagerClient::NetworkManagerClient()
        : impl_(new Impl())
    {
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

}