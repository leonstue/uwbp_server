#include "../header/HttpServer.h"
#include "../rest/header/RestRequestHandler.h"
#include "../rest/header/RestRouter.h"

#include <Poco/Net/HTTPServerParams.h>

#include <iostream>

namespace uwbp::server
{

HttpServer::HttpServer(const RestRouter& router, std::uint16_t port)
    : socket_(port)
{
    auto params = new Poco::Net::HTTPServerParams();
    params->setMaxQueued(64);
    params->setMaxThreads(4);

    server_ = std::make_unique<Poco::Net::HTTPServer>(
        new RestHandlerFactory(router), socket_, params);
}

HttpServer::~HttpServer()
{
    stop();
}

void HttpServer::start()
{
    server_->start();
    std::cout << "HTTP server listening on port "
              << socket_.address().port() << "\n";
}

void HttpServer::stop()
{
    if (server_)
        server_->stop();
}

} // namespace uwbp::server
