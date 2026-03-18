#include "../header/RestRouter.h"

#include <sstream>

namespace uwbp::server
{

std::vector<std::string> RestRouter::splitPath(const std::string& path)
{
    std::vector<std::string> parts;
    std::istringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/'))
    {
        if (!segment.empty())
            parts.push_back(segment);
    }
    return parts;
}

bool RestRouter::isParam(const std::string& segment)
{
    return segment.size() >= 3 && segment.front() == '{' && segment.back() == '}';
}

std::string RestRouter::paramName(const std::string& segment)
{
    return segment.substr(1, segment.size() - 2);
}

void RestRouter::addRoute(const std::string& method, const std::string& path,
                          const std::string& description, RouteHandler handler)
{
    routeInfos_.push_back({method, path, description});

    // check if path contains any {param} segments
    bool hasParams = path.find('{') != std::string::npos;

    if (hasParams)
    {
        Route r;
        r.method = method;
        r.pattern = path;
        r.segments = splitPath(path);
        r.handler = std::move(handler);
        patternRoutes_.push_back(std::move(r));
    }
    else
    {
        exactRoutes_[method + " " + path] = std::move(handler);
    }
}

RouteHandler RestRouter::getHandler(const std::string& method,
                                    const std::string& path,
                                    std::map<std::string, std::string>& params) const
{
    // fast path: exact match
    auto it = exactRoutes_.find(method + " " + path);
    if (it != exactRoutes_.end())
        return it->second;

    // slow path: check pattern routes
    auto requestSegments = splitPath(path);

    for (const auto& route : patternRoutes_)
    {
        if (route.method != method) continue;
        if (route.segments.size() != requestSegments.size()) continue;

        std::map<std::string, std::string> extracted;
        bool match = true;

        for (std::size_t i = 0; i < route.segments.size(); ++i)
        {
            if (isParam(route.segments[i]))
            {
                extracted[paramName(route.segments[i])] = requestSegments[i];
            }
            else if (route.segments[i] != requestSegments[i])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            params = std::move(extracted);
            return route.handler;
        }
    }

    return nullptr;
}

const std::vector<RouteInfo>& RestRouter::allRoutes() const
{
    return routeInfos_;
}

} // namespace uwbp::server
