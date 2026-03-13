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

    // returns nullptr if no matching route
    RouteHandler getHandler(const std::string& method,
                            const std::string& path) const;

    const std::vector<RouteInfo>& allRoutes() const;

private:
    // key is "METHOD /path"
    std::map<std::string, RouteHandler> routes_;
    std::vector<RouteInfo> routeInfos_;
};

} // namespace uwbp::server
