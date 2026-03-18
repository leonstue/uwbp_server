#pragma once

#include "RestRouter.h"
#include "RestHelper.hpp"
#include "DeviceManager.hpp"
#include "UwbDevice.hpp"

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

#include <memory>

namespace uwbp::server
{

inline void registerAnchorRoutes(RestRouter& router,
                                 std::shared_ptr<uwbp::uwb::DeviceManager> dm)
{
    router.addRoute("GET", "/api/anchors", "list all anchors",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto arr = new Poco::JSON::Array();
            for (const auto& dev : dm->getAnchors())
                arr->add(dev.toJson());

            auto res = new Poco::JSON::Object();
            res->set("anchors", Poco::JSON::Array::Ptr(arr));
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("POST", "/api/anchors/master", "set the master anchor",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto id = body->optValue<std::string>("id", "");
            if (id.empty())
                return errorJson("need 'id'");

            if (!dm->setMasterAnchor(id))
                return errorJson("anchor not found");

            return okJson();
        });

    router.addRoute("GET", "/api/anchors/master", "get the current master anchor",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto masterId = dm->getMasterAnchorId();
            if (!masterId)
                return errorJson("no master anchor set");

            auto dev = dm->getDevice(*masterId);
            return dev->toJson();
        });

    router.addRoute("PUT", "/api/anchors/{id}/position", "set anchor position in room",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto id = body->optValue<std::string>("_id", "");

            uwbp::uwb::Vec3 pos;
            pos.x = body->optValue("x", 0.0);
            pos.y = body->optValue("y", 0.0);
            pos.z = body->optValue("z", 0.0);

            if (!dm->setAnchorPosition(id, pos))
                return errorJson("anchor not found");

            auto dev = dm->getDevice(id);
            return dev->toJson();
        });

    // ranging data from the master anchor
    router.addRoute("POST", "/api/ranging", "submit UWB ranging data from master anchor",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto frame = uwbp::uwb::RangingFrame::fromJson(body);

            if (frame.measurements.empty())
                return errorJson("no measurements in frame");

            auto positions = dm->ingestRangingFrame(frame);

            auto arr = new Poco::JSON::Array();
            for (const auto& tp : positions)
                arr->add(tp.toJson());

            auto res = new Poco::JSON::Object();
            res->set("positions", Poco::JSON::Array::Ptr(arr));
            res->set("computed", static_cast<int>(positions.size()));
            return Poco::JSON::Object::Ptr(res);
        });
}

} // namespace uwbp::server
