#pragma once

#include <functional>
#include <map>
#include <string>

#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

namespace uwbp::server
{

using RouteHandler = std::function<
    Poco::JSON::Object::Ptr(Poco::Net::HTTPServerRequest&,
                            const Poco::JSON::Object::Ptr& body)>;

class RestRouter
{
public:
    void addRoute(const std::string& method, const std::string& path,
                  RouteHandler handler);

    // returns nullptr if no matching route
    RouteHandler getHandler(const std::string& method,
                            const std::string& path) const;

private:
    // key is "METHOD /path", e.g. "GET /api/health"
    std::map<std::string, RouteHandler> routes_;
};

} // namespace uwbp::server
