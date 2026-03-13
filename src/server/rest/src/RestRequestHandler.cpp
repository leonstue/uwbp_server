#include "../header/RestRequestHandler.h"
#include "../header/RestRouter.h"

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace uwbp::server
{

// ---- RestRequestHandler ----

RestRequestHandler::RestRequestHandler(const RestRouter& router)
    : router_(router)
{
}

void RestRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                       Poco::Net::HTTPServerResponse& response)
{
    // try to parse json body if there is one
    Poco::JSON::Object::Ptr jsonBody;
    try
    {
        auto parsed = Poco::JSON::Parser().parse(request.stream());
        jsonBody = parsed.extract<Poco::JSON::Object::Ptr>();
    }
    catch (...)
    {
        jsonBody = new Poco::JSON::Object();
    }

    Poco::JSON::Object::Ptr jsonResponse;

    auto handler = router_.getHandler(request.getMethod(), request.getURI());
    if (handler)
    {
        jsonResponse = handler(request, jsonBody);
    }
    else
    {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        jsonResponse = new Poco::JSON::Object();
        jsonResponse->set("error", "not found");
    }

    response.setContentType("application/json");
    std::ostream& ostr = response.send();
    Poco::JSON::Stringifier::stringify(jsonResponse, ostr);
}

// ---- RestHandlerFactory ----

RestHandlerFactory::RestHandlerFactory(const RestRouter& router)
    : router_(router)
{
}

Poco::Net::HTTPRequestHandler*
RestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest&)
{
    return new RestRequestHandler(router_);
}

} // namespace uwbp::server
