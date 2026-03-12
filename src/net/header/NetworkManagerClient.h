#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace uwbp::net
{

struct ApConfig; // forward declaration

struct NmDeviceInfo
{
    std::string objectPath;   // z.B. /org/freedesktop/NetworkManager/Devices/2
    std::string iface;        // z.B. wlan0
    std::uint32_t deviceType; // NM DeviceType (WiFi etc.)
    std::uint32_t state;      // NM Device State
};

// Ergebnis von createAp -- wird fuer Cleanup benoetigt
struct NmActiveAp
{
    std::string connectionPath;       // Settings-Connection Objekt-Pfad
    std::string activeConnectionPath; // Active-Connection Objekt-Pfad
};

class NetworkManagerClient
{
public:
    NetworkManagerClient();
    ~NetworkManagerClient();

    std::uint32_t getState() const;                 // NetworkManager global State
    std::vector<NmDeviceInfo> listDevices() const;  // alle Devices

    // Convenience: findet device by Interface-Name (z.B. "wlan0"), wirft nicht
    bool tryGetDeviceByIface(const std::string& iface, NmDeviceInfo& out) const;

    // --- AP Management ---

    // Erstellt und aktiviert einen WiFi AP. Wirft std::runtime_error bei Fehler.
    NmActiveAp createAp(const ApConfig& cfg);

    // Deaktiviert und loescht einen AP. Safe bei bereits entferntem AP.
    void removeAp(const NmActiveAp& ap);

    // Entfernt alle via createAp erstellten APs.
    void removeAllAps();

    // Liste der aktuell aktiven APs (fuer State-File Serialisierung).
    const std::vector<NmActiveAp>& activeAps() const;

private:
    // PImpl-light: wir vermeiden das sdbus include hier (Header bleibt clean)
    struct Impl;
    Impl* impl_;
};

} // namespace uwbp::net
