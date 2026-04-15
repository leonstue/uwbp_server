#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>

namespace uwbp::uwb
{

struct Vec3
{
    double x = 0.0, y = 0.0, z = 0.0;

    Poco::JSON::Object::Ptr toJson() const
    {
        auto o = new Poco::JSON::Object();
        o->set("x", x);
        o->set("y", y);
        o->set("z", z);
        return Poco::JSON::Object::Ptr(o);
    }

    static Vec3 fromJson(const Poco::JSON::Object::Ptr& o)
    {
        return {
            o->optValue("x", 0.0),
            o->optValue("y", 0.0),
            o->optValue("z", 0.0)
        };
    }
};

enum class DeviceType { Anchor, Tag };

inline std::string deviceTypeToString(DeviceType t)
{
    return t == DeviceType::Anchor ? "anchor" : "tag";
}

inline DeviceType deviceTypeFromString(const std::string& s)
{
    return s == "anchor" ? DeviceType::Anchor : DeviceType::Tag;
}

struct UwbDevice
{
    std::string id;
    DeviceType type = DeviceType::Tag;
    std::string name;
    std::string color = "#FFFFFF";
    Vec3 position; // for anchors: known position, for tags: last computed

    Poco::JSON::Object::Ptr toJson() const
    {
        auto o = new Poco::JSON::Object();
        o->set("id", id);
        o->set("type", deviceTypeToString(type));
        o->set("name", name);
        o->set("color", color);
        o->set("position", position.toJson());
        return Poco::JSON::Object::Ptr(o);
    }
};

// single distance measurement from one anchor to one tag
struct RangingMeasurement
{
    std::string anchorId;
    std::string tagId;
    double distance; // meters (from SS-TWR)
    std::uint64_t timestamp = 0; // millis, used for time-window grouping

    static RangingMeasurement fromJson(const Poco::JSON::Object::Ptr& o)
    {
        return {
            o->optValue<std::string>("anchorId", ""),
            o->optValue<std::string>("tagId", ""),
            o->optValue("distance", 0.0),
            static_cast<std::uint64_t>(o->optValue<double>("timestamp", 0.0))
        };
    }
};

// a ranging post from a single anchor (can contain multiple tag measurements)
struct RangingFrame
{
    std::uint64_t timestamp = 0;
    std::vector<RangingMeasurement> measurements;

    static RangingFrame fromJson(const Poco::JSON::Object::Ptr& o)
    {
        RangingFrame f;
        f.timestamp = static_cast<std::uint64_t>(
            o->optValue<double>("timestamp", 0.0));

        auto arr = o->getArray("measurements");
        if (arr)
        {
            for (std::size_t i = 0; i < arr->size(); ++i)
            {
                auto m = arr->getObject(i);
                if (m)
                {
                    auto meas = RangingMeasurement::fromJson(m);
                    // use frame timestamp if measurement doesnt have its own
                    if (meas.timestamp == 0) meas.timestamp = f.timestamp;
                    f.measurements.push_back(std::move(meas));
                }
            }
        }
        return f;
    }
};

// computed position result for a tag
struct TagPosition
{
    std::string tagId;
    Vec3 position;
    std::uint64_t timestamp = 0;
    double residual = 0.0; // quality metric, lower = better

    Poco::JSON::Object::Ptr toJson() const
    {
        auto o = new Poco::JSON::Object();
        o->set("tagId", tagId);
        o->set("position", position.toJson());
        o->set("timestamp", static_cast<double>(timestamp));
        o->set("residual", residual);
        return Poco::JSON::Object::Ptr(o);
    }
};

} // namespace uwbp::uwb
