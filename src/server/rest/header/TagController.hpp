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

inline void registerTagRoutes(RestRouter& router,
                              std::shared_ptr<uwbp::uwb::DeviceManager> dm)
{
    router.addRoute("GET", "/api/tags", "list all tags",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto arr = new Poco::JSON::Array();
            for (const auto& dev : dm->getTags())
                arr->add(dev.toJson());

            auto res = new Poco::JSON::Object();
            res->set("tags", Poco::JSON::Array::Ptr(arr));
            return Poco::JSON::Object::Ptr(res);
        });

    // position queries
    router.addRoute("GET", "/api/positions", "get latest positions of all tags",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto arr = new Poco::JSON::Array();
            for (const auto& tp : dm->getAllLatestPositions())
                arr->add(tp.toJson());

            auto res = new Poco::JSON::Object();
            res->set("positions", Poco::JSON::Array::Ptr(arr));
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("GET", "/api/positions/{tagId}", "get latest position of a specific tag",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto tagId = body->optValue<std::string>("_tagId", "");
            auto pos = dm->getLatestPosition(tagId);
            if (!pos) return errorJson("no position data for this tag");
            return pos->toJson();
        });
}

} // namespace uwbp::server
