#include "../header/RestRouter.h"

namespace uwbp::server
{

void RestRouter::addRoute(const std::string& method, const std::string& path,
                          const std::string& description, RouteHandler handler)
{
    routes_[method + " " + path] = std::move(handler);
    routeInfos_.push_back({method, path, description});
}

RouteHandler RestRouter::getHandler(const std::string& method,
                                    const std::string& path) const
{
    auto it = routes_.find(method + " " + path);
    if (it != routes_.end())
        return it->second;
    return nullptr;
}

const std::vector<RouteInfo>& RestRouter::allRoutes() const
{
    return routeInfos_;
}

} // namespace uwbp::server
