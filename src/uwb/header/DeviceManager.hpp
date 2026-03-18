#pragma once

#include "UwbDevice.hpp"
#include "Trilateration.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace uwbp::uwb
{

class DeviceManager
{
public:
    // ---- device registration (called by ESP32s) ----

    bool registerDevice(const std::string& id, DeviceType type)
    {
        std::lock_guard lock(mutex_);
        if (devices_.contains(id)) return false; // already registered

        UwbDevice dev;
        dev.id = id;
        dev.type = type;
        dev.name = id; // default name is the id
        devices_[id] = std::move(dev);
        return true;
    }

    std::optional<UwbDevice> getDevice(const std::string& id) const
    {
        std::lock_guard lock(mutex_);
        auto it = devices_.find(id);
        if (it == devices_.end()) return std::nullopt;
        return it->second;
    }

    std::vector<UwbDevice> getAllDevices() const
    {
        std::lock_guard lock(mutex_);
        std::vector<UwbDevice> result;
        result.reserve(devices_.size());
        for (const auto& [_, dev] : devices_)
            result.push_back(dev);
        return result;
    }

    std::vector<UwbDevice> getAnchors() const
    {
        std::lock_guard lock(mutex_);
        std::vector<UwbDevice> result;
        for (const auto& [_, dev] : devices_)
            if (dev.type == DeviceType::Anchor) result.push_back(dev);
        return result;
    }

    std::vector<UwbDevice> getTags() const
    {
        std::lock_guard lock(mutex_);
        std::vector<UwbDevice> result;
        for (const auto& [_, dev] : devices_)
            if (dev.type == DeviceType::Tag) result.push_back(dev);
        return result;
    }

    // ---- frontend configuration ----

    bool updateDevice(const std::string& id,
                      const std::string& name,
                      const std::string& color)
    {
        std::lock_guard lock(mutex_);
        auto it = devices_.find(id);
        if (it == devices_.end()) return false;
        if (!name.empty()) it->second.name = name;
        if (!color.empty()) it->second.color = color;
        return true;
    }

    bool setMasterAnchor(const std::string& anchorId)
    {
        std::lock_guard lock(mutex_);
        auto it = devices_.find(anchorId);
        if (it == devices_.end() || it->second.type != DeviceType::Anchor)
            return false;

        // clear old master
        for (auto& [_, dev] : devices_)
            dev.isMaster = false;

        it->second.isMaster = true;
        return true;
    }

    std::optional<std::string> getMasterAnchorId() const
    {
        std::lock_guard lock(mutex_);
        for (const auto& [id, dev] : devices_)
            if (dev.isMaster) return id;
        return std::nullopt;
    }

    bool setAnchorPosition(const std::string& anchorId, const Vec3& pos)
    {
        std::lock_guard lock(mutex_);
        auto it = devices_.find(anchorId);
        if (it == devices_.end() || it->second.type != DeviceType::Anchor)
            return false;
        it->second.position = pos;
        return true;
    }

    // ---- ranging data ----

    // ingest a ranging frame and compute tag positions.
    // returns computed positions for all tags that had enough data.
    std::vector<TagPosition> ingestRangingFrame(const RangingFrame& frame)
    {
        std::lock_guard lock(mutex_);

        // group measurements by tag
        std::unordered_map<std::string, std::vector<AnchorDistance>> tagMeasurements;

        for (const auto& m : frame.measurements)
        {
            // check that the anchor exists and we know its position
            auto anchorIt = devices_.find(m.anchorId);
            if (anchorIt == devices_.end() || anchorIt->second.type != DeviceType::Anchor)
                continue;

            // check tag exists
            auto tagIt = devices_.find(m.tagId);
            if (tagIt == devices_.end() || tagIt->second.type != DeviceType::Tag)
                continue;

            tagMeasurements[m.tagId].push_back({
                anchorIt->second.position,
                m.distance
            });
        }

        // trilaterate each tag
        std::vector<TagPosition> results;
        for (auto& [tagId, ads] : tagMeasurements)
        {
            if (ads.size() < 3) continue;

            auto solved = trilaterate(ads);
            if (!solved) continue;

            TagPosition tp;
            tp.tagId = tagId;
            tp.position = solved->first;
            tp.timestamp = frame.timestamp;
            tp.residual = solved->second;

            // store in latest positions map
            latestPositions_[tagId] = tp;

            // also update the device's position field
            auto devIt = devices_.find(tagId);
            if (devIt != devices_.end())
                devIt->second.position = tp.position;

            results.push_back(std::move(tp));
        }

        return results;
    }

    std::optional<TagPosition> getLatestPosition(const std::string& tagId) const
    {
        std::lock_guard lock(mutex_);
        auto it = latestPositions_.find(tagId);
        if (it == latestPositions_.end()) return std::nullopt;
        return it->second;
    }

    std::vector<TagPosition> getAllLatestPositions() const
    {
        std::lock_guard lock(mutex_);
        std::vector<TagPosition> result;
        result.reserve(latestPositions_.size());
        for (const auto& [_, tp] : latestPositions_)
            result.push_back(tp);
        return result;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, UwbDevice> devices_;
    std::unordered_map<std::string, TagPosition> latestPositions_;
};

} // namespace uwbp::uwb
