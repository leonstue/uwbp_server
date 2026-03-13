#include "../header/RestRouter.h"

namespace uwbp::server
{

void RestRouter::addRoute(const std::string& method, const std::string& path,
                          RouteHandler handler)
{
    routes_[method + " " + path] = std::move(handler);
}

RouteHandler RestRouter::getHandler(const std::string& method,
                                    const std::string& path) const
{
    auto it = routes_.find(method + " " + path);
    if (it != routes_.end())
        return it->second;
    return nullptr;
}

} // namespace uwbp::server
