#pragma once

#include <cstdint>
#include <string>

namespace uwbp::net
{

struct ApConfig
{
    std::string ssid;
    std::string psk;            // WPA-PSK Passphrase (8-63 Zeichen)
    std::string iface;          // WiFi Interface, z.B. "wlan0"
    std::string band = "bg";    // "bg" = 2.4 GHz, "a" = 5 GHz
    std::uint32_t channel = 6;
    bool hidden = false;

    // Vorkonfigurierte APs
    static ApConfig espNetwork(const std::string& psk,
                               const std::string& iface = "wlan0");
    static ApConfig userNetwork(const std::string& psk,
                                const std::string& iface = "wlan0");
};

} // namespace uwbp::net
