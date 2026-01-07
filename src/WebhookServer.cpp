#include "WebhookServer.hpp"
#include <iostream>

WebhookServer::WebhookServer(asio::io_context &io, unsigned short port, RequestHandler handler)
    : handler_(handler), io_(io), acceptor_(io, tcp::endpoint(tcp::v4(), port))
{
    accept();
}

void WebhookServer::accept()
{
    auto socket = std::make_shared<tcp::socket>(io_);
    acceptor_.async_accept(*socket, [this, socket](beast::error_code ec) {
        if (!ec) handle_connection(socket);
        accept();
    });
}

void WebhookServer::handle_connection(std::shared_ptr<tcp::socket> socket)
{
    auto buffer  = std::make_shared<beast::flat_buffer>();
    auto request = std::make_shared<http::request<http::string_body>>();

    http::async_read(*socket, *buffer, *request, [this, socket, buffer, request](beast::error_code ec, std::size_t) {
        if (!ec) { process_request(socket, *request); }
    });
}

void WebhookServer::process_request(std::shared_ptr<tcp::socket> socket, const http::request<http::string_body> &req)
{
    auto res      = handler_(req, socket->remote_endpoint().address());
    auto response = std::make_shared<http::response<http::string_body>>(
        res.first ? http::status::ok : http::status::forbidden, req.version());
    if (!res.first) std::cout << "[WebhookServer] Bad Request " << req << "\n\n[WebhookServer] What:" << res.second;
    // response->set(http::field::content_type, "text/plain");
    response->set(http::field::content_type, "text/html; charset=utf-8");
    response->body() = res.first ? res.second : "Bad Request";
    response->prepare_payload();
    response->keep_alive(false);
    http::async_write(*socket, *response, [socket, response](beast::error_code, std::size_t) {
        beast::error_code ec;
        ec = socket->shutdown(tcp::socket::shutdown_send, ec);
        if (ec && ec != boost::asio::error::not_connected) std::cout << "[WebhookServer] Error on close Session" << ec;
        ec = socket->close(ec);
    });
}
