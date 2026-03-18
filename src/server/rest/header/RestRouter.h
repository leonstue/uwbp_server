#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPServerRequest.h>

namespace uwbp::server
{

using RouteHandler = std::function<
    Poco::JSON::Object::Ptr(Poco::Net::HTTPServerRequest&,
                            const Poco::JSON::Object::Ptr& body)>;

struct RouteInfo
{
    std::string method;
    std::string path;
    std::string description;
};

class RestRouter
{
public:
    void addRoute(const std::string& method, const std::string& path,
                  const std::string& description, RouteHandler handler);

    // returns nullptr if no matching route.
    // if the route has path params like {id}, they get injected into
    // the params map (e.g. "id" -> "some-value")
    RouteHandler getHandler(const std::string& method,
                            const std::string& path,
                            std::map<std::string, std::string>& params) const;

    const std::vector<RouteInfo>& allRoutes() const;

private:
    struct Route
    {
        std::string method;
        std::string pattern; // e.g. "/api/devices/{id}"
        std::vector<std::string> segments; // split by /
        RouteHandler handler;
    };

    // exact match map for fast lookup (routes without params)
    std::map<std::string, RouteHandler> exactRoutes_;
    // pattern routes (routes with {param} segments)
    std::vector<Route> patternRoutes_;

    std::vector<RouteInfo> routeInfos_;

    static std::vector<std::string> splitPath(const std::string& path);
    static bool isParam(const std::string& segment);
    static std::string paramName(const std::string& segment);
};

} // namespace uwbp::server
