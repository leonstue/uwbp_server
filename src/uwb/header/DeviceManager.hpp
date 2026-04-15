#pragma once

#include "UwbDevice.hpp"
#include "Trilateration.hpp"

#include <chrono>
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
    // time window in ms - measurements older than this get discarded
    static constexpr std::uint64_t AGGREGATION_WINDOW_MS = 100;

    // ---- device registration (called by ESP32s) ----

    bool registerDevice(const std::string& id, DeviceType type)
    {
        std::lock_guard lock(mutex_);
        if (devices_.contains(id)) return false;

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

    bool setAnchorPosition(const std::string& anchorId, const Vec3& pos)
    {
        std::lock_guard lock(mutex_);
        auto it = devices_.find(anchorId);
        if (it == devices_.end() || it->second.type != DeviceType::Anchor)
            return false;
        it->second.position = pos;
        return true;
    }

    // ---- ranging data (each anchor posts individually) ----

    // ingest measurements from a single anchor.
    // adds to the sliding window buffer, then checks if we can
    // trilaterate any tags (>= 3 different anchors within the time window).
    std::vector<TagPosition> ingestMeasurements(const RangingFrame& frame)
    {
        std::lock_guard lock(mutex_);

        auto now = frame.timestamp;
        if (now == 0)
        {
            // fallback: use server time if anchor didnt send a timestamp
            now = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
        }

        // add new measurements to the buffer
        for (const auto& m : frame.measurements)
        {
            // validate anchor + tag exist
            auto anchorIt = devices_.find(m.anchorId);
            if (anchorIt == devices_.end() || anchorIt->second.type != DeviceType::Anchor)
                continue;

            auto tagIt = devices_.find(m.tagId);
            if (tagIt == devices_.end() || tagIt->second.type != DeviceType::Tag)
                continue;

            BufferedMeasurement bm;
            bm.anchorId = m.anchorId;
            bm.tagId = m.tagId;
            bm.distance = m.distance;
            bm.timestamp = (m.timestamp != 0) ? m.timestamp : now;

            measurementBuffer_[m.tagId].push_back(std::move(bm));
        }

        // now try to trilaterate each tag that has buffered data
        std::vector<TagPosition> results;

        for (auto& [tagId, buffer] : measurementBuffer_)
        {
            // purge old measurements outside the window
            auto cutoff = (now > AGGREGATION_WINDOW_MS) ? (now - AGGREGATION_WINDOW_MS) : 0;
            std::erase_if(buffer, [cutoff](const BufferedMeasurement& bm) {
                return bm.timestamp < cutoff;
            });

            if (buffer.empty()) continue;

            // collect latest measurement per anchor (deduplicate)
            std::unordered_map<std::string, BufferedMeasurement> latestPerAnchor;
            for (const auto& bm : buffer)
            {
                auto it = latestPerAnchor.find(bm.anchorId);
                if (it == latestPerAnchor.end() || bm.timestamp > it->second.timestamp)
                    latestPerAnchor[bm.anchorId] = bm;
            }

            // need at least 3 different anchors
            if (latestPerAnchor.size() < 3) continue;

            // build anchor-distance pairs for trilateration
            std::vector<AnchorDistance> ads;
            for (const auto& [anchorId, bm] : latestPerAnchor)
            {
                auto anchorIt = devices_.find(anchorId);
                if (anchorIt == devices_.end()) continue;

                ads.push_back({anchorIt->second.position, bm.distance});
            }

            if (ads.size() < 3) continue;

            auto solved = trilaterate(ads);
            if (!solved) continue;

            TagPosition tp;
            tp.tagId = tagId;
            tp.position = solved->first;
            tp.timestamp = now;
            tp.residual = solved->second;

            latestPositions_[tagId] = tp;

            // update device position too
            auto devIt = devices_.find(tagId);
            if (devIt != devices_.end())
                devIt->second.position = tp.position;

            results.push_back(std::move(tp));

            // clear buffer for this tag after successful trilateration
            buffer.clear();
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
    struct BufferedMeasurement
    {
        std::string anchorId;
        std::string tagId;
        double distance;
        std::uint64_t timestamp;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, UwbDevice> devices_;
    std::unordered_map<std::string, TagPosition> latestPositions_;

    // sliding window buffer: tagId -> list of recent measurements
    std::unordered_map<std::string, std::vector<BufferedMeasurement>> measurementBuffer_;
};

} // namespace uwbp::uwb
