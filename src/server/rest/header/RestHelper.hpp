#pragma once

#include <Poco/JSON/Object.h>
#include <string>

namespace uwbp::server
{

inline Poco::JSON::Object::Ptr errorJson(const std::string& msg)
{
    auto res = new Poco::JSON::Object();
    res->set("error", msg);
    return Poco::JSON::Object::Ptr(res);
}

inline Poco::JSON::Object::Ptr okJson()
{
    auto res = new Poco::JSON::Object();
    res->set("ok", true);
    return Poco::JSON::Object::Ptr(res);
}

} // namespace uwbp::server
