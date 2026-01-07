#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <set>
#include <iostream>
#include <utility>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = asio::ip::tcp;
using address   = asio::ip::address;

class WebhookServer
{
public:
    using RequestHandler =
        std::function<std::pair<bool, std::string>(const http::request<http::string_body> &, const address &client_ip)>;

    WebhookServer(asio::io_context &io, unsigned short port, RequestHandler handler);

private:
    void accept();

    void handle_connection(std::shared_ptr<tcp::socket> socket);

    void process_request(std::shared_ptr<tcp::socket> socket, const http::request<http::string_body> &req);

private:
    RequestHandler handler_;
    asio::io_context &io_;
    tcp::acceptor acceptor_;
};
