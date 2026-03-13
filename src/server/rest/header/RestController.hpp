#pragma once

#include "RestRouter.h"

#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

namespace uwbp::server
{

// register all routes on the given router
inline void registerRoutes(RestRouter& router)
{
    router.addRoute("GET", "/api/health",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("status", "ok");
            return Poco::JSON::Object::Ptr(res);
        });

    // placeholder, will be replaced once device manager exists
    router.addRoute("GET", "/api/devices",
        [](Poco::Net::HTTPServerRequest&, const Poco::JSON::Object::Ptr&)
        {
            auto res = new Poco::JSON::Object();
            res->set("devices", Poco::JSON::Array::Ptr(new Poco::JSON::Array()));
            return Poco::JSON::Object::Ptr(res);
        });
}

} // namespace uwbp::server
