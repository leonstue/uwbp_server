#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace uwbp::net
{

struct ApConfig;

struct NmDeviceInfo
{
    std::string objectPath;   // e.g. /org/freedesktop/NetworkManager/Devices/2
    std::string iface;        // e.g. wlan0
    std::uint32_t deviceType; // NM_DEVICE_TYPE enum
    std::uint32_t state;
};

// returned by createAp, needed for cleanup later
struct NmActiveAp
{
    std::string connectionPath;       // settings connection obj path
    std::string activeConnectionPath; // active conection obj path
};

class NetworkManagerClient
{
public:
    NetworkManagerClient();
    ~NetworkManagerClient();

    std::uint32_t getState() const;
    std::vector<NmDeviceInfo> listDevices() const;

    // find device by interface name, returns false if not found (doesnt throw)
    bool tryGetDeviceByIface(const std::string& iface, NmDeviceInfo& out) const;

    // --- AP management ---

    // creates and activates a wifi AP, throws on failure
    NmActiveAp createAp(const ApConfig& cfg);

    // deactivate + delete an AP. safe to call if already gone
    void removeAp(const NmActiveAp& ap);

    void removeAllAps();

    // exposed for state file serialization
    const std::vector<NmActiveAp>& activeAps() const;

private:
    // pimpl to keep sdbus out of the header
    struct Impl;
    Impl* impl_;
};

} // namespace uwbp::net
