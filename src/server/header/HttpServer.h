#pragma once

#include <cstdint>
#include <memory>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/ServerSocket.h>

namespace uwbp::server
{

class RestRouter;

class HttpServer
{
public:
    HttpServer(const RestRouter& router, std::uint16_t port);
    ~HttpServer();

    void start();
    void stop();

private:
    Poco::Net::ServerSocket socket_;
    std::unique_ptr<Poco::Net::HTTPServer> server_;
};

} // namespace uwbp::server
