#pragma once

#include "RestRouter.h"

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

#include <csignal>

namespace uwbp::server
{

// register all routes. pass in a pointer to the running flag
// so the shutdown endpoint can trigger a clean exit.
inline void registerRoutes(RestRouter& router, volatile sig_atomic_t* running)
{
    router.addRoute("GET", "/api/health",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("status", "ok");
            return Poco::JSON::Object::Ptr(res);
        });

    // placeholder, will be hooked up to device manager later
    router.addRoute("GET", "/api/devices",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("devices", Poco::JSON::Array::Ptr(new Poco::JSON::Array()));
            return Poco::JSON::Object::Ptr(res);
        });

    router.addRoute("POST", "/api/shutdown",
        [running](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            *running = 0;
            // send SIGTERM to ourselves so pause() wakes up
            kill(getpid(), SIGTERM);

            auto res = new Poco::JSON::Object();
            res->set("message", "shutting down");
            return Poco::JSON::Object::Ptr(res);
        });
}

} // namespace uwbp::server
