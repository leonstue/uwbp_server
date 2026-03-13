#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>

namespace uwbp::server
{

class RestRouter;

class RestRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    explicit RestRequestHandler(const RestRouter& router);

    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override;

private:
    const RestRouter& router_;
};

class RestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    explicit RestHandlerFactory(const RestRouter& router);

    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& request) override;

private:
    const RestRouter& router_;
};

} // namespace uwbp::server
