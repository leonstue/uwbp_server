#pragma once

#include <cstdint>
#include <string>

namespace uwbp::net
{

struct ApConfig
{
    std::string ssid;
    std::string psk;            // WPA-PSK passphrase (8-63 chars)
    std::string iface;          // wifi interface, e.g. "wlan0"
    std::string band = "bg";    // "bg" = 2.4GHz, "a" = 5GHz
    std::uint32_t channel = 6;
    bool hidden = false;

    static ApConfig espNetwork(const std::string& psk,
                               const std::string& iface = "wlan0");
    static ApConfig userNetwork(const std::string& psk,
                                const std::string& iface = "wlan0");
};

} // namespace uwbp::net
