#include "../header/ApConfig.h"

namespace uwbp::net
{

ApConfig ApConfig::espNetwork(const std::string& psk, const std::string& iface)
{
    ApConfig cfg;
    cfg.ssid    = "UWBP-ESP";
    cfg.psk     = psk;
    cfg.iface   = iface;
    cfg.band    = "bg";
    cfg.channel = 6;
    cfg.hidden  = true;
    return cfg;
}

ApConfig ApConfig::userNetwork(const std::string& psk, const std::string& iface)
{
    ApConfig cfg;
    cfg.ssid    = "UWBP-User";
    cfg.psk     = psk;
    cfg.iface   = iface;
    cfg.band    = "bg";
    cfg.channel = 6;
    cfg.hidden  = false;
    return cfg;
}

} // namespace uwbp::net
