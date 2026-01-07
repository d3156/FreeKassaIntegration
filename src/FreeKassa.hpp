#pragma once
#include "WebhookServer.hpp"
#include <boost/asio.hpp>
#include <boost/beast/http/verb.hpp>

#include <map>
#include <memory>
#include <string>

namespace FreeKassa
{
    using namespace boost;
    using json_map = std::map<std::string, std::string>;

    class Client
    {
        const std::string shop_id_     = "";
        const std::string secretword1  = "";
        const std::string secretword2  = "";
        const std::string shop_link    = "";
        const std::string support_link = "";
        int port_                      = 8753;

    public:
        Client(boost::asio::io_context &io);

        std::string createFreeKassaLink(double amount, const std::string &paymentId);

        std::pair<bool, std::string> handle_webhook_request(const http::request<http::string_body> &req,
                                                            const address &client_ip);

    private:
        std::string htmlPayPage = "";
        std::string renderAnswer(const http::request<http::string_body> &req);
        std::map<std::string, std::string> amountByMerchant;
        std::unique_ptr<WebhookServer> server;
    };

}
