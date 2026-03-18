#pragma once

#include "RestRouter.h"
#include "RestHelper.hpp"
#include "AnchorController.hpp"
#include "TagController.hpp"
#include "DeviceManager.hpp"
#include "UwbDevice.hpp"

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

#include <csignal>
#include <memory>

namespace uwbp::server
{

inline void registerRoutes(RestRouter& router,
                           volatile sig_atomic_t* running,
                           std::shared_ptr<uwbp::uwb::DeviceManager> dm)
{
    // ---- general ----

    router.addRoute("GET", "/api/health", "server health check",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("status", "ok");
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("POST", "/api/shutdown", "gracefully shut down the server",
        [running](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            *running = 0;
            kill(getpid(), SIGTERM);

            auto res = new Poco::JSON::Object();
            res->set("message", "shutting down");
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("GET", "/api", "list all available API endpoints",
        [&router](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto arr = new Poco::JSON::Array();
            for (const auto& r : router.allRoutes())
            {
                auto obj = new Poco::JSON::Object();
                obj->set("method", r.method);
                obj->set("path", r.path);
                obj->set("description", r.description);
                arr->add(Poco::JSON::Object::Ptr(obj));
            }

            auto res = new Poco::JSON::Object();
            res->set("routes", Poco::JSON::Array::Ptr(arr));
            return Poco::JSON::Object::Ptr(res);
        });

    // ---- device registration + listing (used by both ESPs and frontend) ----

    router.addRoute("POST", "/api/devices/register", "register a new ESP32 device",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto id = body->optValue<std::string>("id", "");
            auto typeStr = body->optValue<std::string>("type", "");

            if (id.empty() || (typeStr != "anchor" && typeStr != "tag"))
                return errorJson("need 'id' and 'type' (anchor/tag)");

            auto type = uwbp::uwb::deviceTypeFromString(typeStr);
            bool created = dm->registerDevice(id, type);

            if (!created)
                return errorJson("device already registered");

            return okJson();
        });

    router.addRoute("GET", "/api/devices", "list all registered devices",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto arr = new Poco::JSON::Array();
            for (const auto& dev : dm->getAllDevices())
                arr->add(dev.toJson());

            auto res = new Poco::JSON::Object();
            res->set("devices", Poco::JSON::Array::Ptr(arr));
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("GET", "/api/devices/{id}", "get a single device by id",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto id = body->optValue<std::string>("_id", "");
            auto dev = dm->getDevice(id);
            if (!dev) return errorJson("device not found");
            return dev->toJson();
        });

    router.addRoute("PUT", "/api/devices/{id}", "update device name and color",
        [dm](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr& body)
        {
            auto id = body->optValue<std::string>("_id", "");
            auto name = body->optValue<std::string>("name", "");
            auto color = body->optValue<std::string>("color", "");

            if (!dm->updateDevice(id, name, color))
                return errorJson("device not found");

            auto dev = dm->getDevice(id);
            return dev->toJson();
        });

    // ---- anchor + tag specific routes ----
    registerAnchorRoutes(router, dm);
    registerTagRoutes(router, dm);
}

} // namespace uwbp::server
