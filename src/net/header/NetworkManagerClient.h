#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace uwbp::net
{
struct NmDeviceInfo
{
    std::string objectPath;   // z.B. /org/freedesktop/NetworkManager/Devices/2
    std::string iface;        // z.B. wlan0
    std::uint32_t deviceType; // NM DeviceType (WiFi etc.)
    std::uint32_t state;      // NM Device State
};

class NetworkManagerClient
{
public:
    NetworkManagerClient();

    std::uint32_t getState() const;                 // NetworkManager global State
    std::vector<NmDeviceInfo> listDevices() const;  // alle Devices

    // Convenience: findet device by Interface-Name (z.B. "wlan0"), wirft nicht
    bool tryGetDeviceByIface(const std::string& iface, NmDeviceInfo& out) const;

private:
    // PImpl-light: wir vermeiden das sdbus include hier (Header bleibt clean)
    struct Impl;
    Impl* impl_;
};
}