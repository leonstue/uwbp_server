#pragma once

#include "RestRouter.h"

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

#include <csignal>

namespace uwbp::server
{

inline void registerRoutes(RestRouter& router, volatile sig_atomic_t* running)
{
    router.addRoute("GET", "/api/health", "server health check",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("status", "ok");
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("GET", "/api/devices", "list connected ESP32 devices",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("devices", Poco::JSON::Array::Ptr(new Poco::JSON::Array()));
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

    // returns all registered routes with method, path and description
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
}

} // namespace uwbp::server
