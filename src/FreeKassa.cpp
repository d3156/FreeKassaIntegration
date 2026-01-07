#include "FreeKassa.hpp"
#include "WebhookServer.hpp"
#include <exception>
#include <filesystem>
#include <format>

#include <boost/algorithm/hex.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <openssl/md5.h> // Для генерации MD5
#include <boost/algorithm/string.hpp>
#include <fstream>

namespace FreeKassa
{

    Client::Client(boost::asio::io_context &io)
        : shop_id_(getenv("FREE_KASSA_SHOP_ID") ? getenv("FREE_KASSA_SHOP_ID") : ""),
          secretword1(getenv("FREE_KASSA_SECRET_WORD1") ? getenv("FREE_KASSA_SECRET_WORD1") : ""),
          secretword2(getenv("FREE_KASSA_SECRET_WORD2") ? getenv("FREE_KASSA_SECRET_WORD2") : ""),
          shop_link(getenv("SHOP_LINK") ? getenv("SHOP_LINK") : ""),
          support_link(getenv("SUPPORT_LINK") ? getenv("SUPPORT_LINK") : ""),
          port_(atoi(getenv("FREE_KASSA_ACCEPT_PORT") ? getenv("FREE_KASSA_ACCEPT_PORT") : "8753"))
    {
        std::cout << "[FreeKassa::Client] HTTP server started on port " << port_ << std::endl;
        std::set<address> whitelist;
        std::vector<std::string> result;
        if (getenv("FREE_KASSA_VALID_IPS")) boost::split(result, getenv("FREE_KASSA_VALID_IPS"), boost::is_any_of(","));
        for (auto i : result) try {
                whitelist.insert(address::from_string(i));
            } catch (std::exception e) {
                std::cout << "[FreeKassa::Client] Error parse FREE_KASSA_VALID_IPS " << e.what() << std::endl;
            }
        auto handler = [this](const http::request<http::string_body> &req,
                              const address &client_ip) -> std::pair<bool, std::string> {
            return this->handle_webhook_request(req, client_ip);
        };
        server = std::make_unique<WebhookServer>(io, port_, handler);
        if (!std::filesystem::exists("./status/Page.html")) {
            std::cout << "[FreeKassa::Client] Error, file ./status/Page.html not found!!!\n";
            exit(-1);
        }
        htmlPayPage = (std::ostringstream() << std::ifstream("./status/Page.html").rdbuf()).str();
    }

    std::string md5(const std::string &s)
    {
        unsigned char d[MD5_DIGEST_LENGTH];
        MD5(reinterpret_cast<const unsigned char *>(s.data()), s.size(), d);
        return boost::algorithm::hex_lower(std::string(reinterpret_cast<char *>(d), MD5_DIGEST_LENGTH));
    }

    std::string Client::createFreeKassaLink(double amount, const std::string &paymentId)
    {
        auto amount_s               = std::format("{:.2f}", amount);
        amountByMerchant[paymentId] = amount_s;
        return "https://pay.fk.money/?m=" + shop_id_ + "&oa=" + amount_s + "&o=" + paymentId +
               "&s=" + md5(shop_id_ + ":" + amount_s + ":" + secretword1 + ":RUB:" + paymentId) + "&currency=RUB";
    }

#include <unordered_map>

    std::unordered_map<std::string, std::string> parse_form(const std::string &body)
    {
        std::unordered_map<std::string, std::string> params;
        size_t start = 0;
        while (start < body.size()) {
            auto end = body.find('&', start);
            if (end == std::string::npos) end = body.size();
            auto eq = body.find('=', start);
            if (eq != std::string::npos && eq < end) {
                auto key    = body.substr(start, eq - start);
                auto val    = body.substr(eq + 1, end - eq - 1);
                params[key] = val;
            }
            start = end + 1;
        }
        return params;
    }

    std::pair<bool, std::string> Client::handle_webhook_request(const http::request<http::string_body> &req,
                                                                const address &client_ip)
    {
        if (req.body() == "status_check=1") return {true, "status_check=1"};

        auto rendered = renderAnswer(req);
        if (!rendered.empty()) return {true, rendered};

        auto p        = parse_form(req.body());
        auto it_sign  = p.find("SIGN");
        auto it_mid   = p.find("MERCHANT_ID");
        auto it_amt   = p.find("AMOUNT");
        auto it_order = p.find("MERCHANT_ORDER_ID");

        if (it_sign == p.end() || it_mid == p.end() || it_amt == p.end() || it_order == p.end())
            return {false, "BadRequest. Not founded SIGN or MERCHANT_ID or AMOUNT or MERCHANT_ORDER_ID"};
        std::string sign = md5(it_mid->second + ":" + it_amt->second + ":" + secretword2 + ":" + it_order->second);
        if (shop_id_ != it_mid->second) return {false, "Incorect MERCHANT_ID"};
        if (sign != it_sign->second) return {false, "Bad signature"};

        /* Place callback here */
        std::cout << "Success payment" << std::endl;
        std::cout << "Wallet: " << it_order->second << std::endl;
        std::cout << "Amount: " << it_amt->second << std::endl;

        return {true, "OK\n"};
    }

    std::string Client::renderAnswer(const http::request<http::string_body> &req)
    {
        // Извлекаем целевой путь из запроса
        std::string target = req.target();

        // Определяем тип страницы (success/fail)
        bool is_success_page = false;
        if (target.find("/payment/success") == 0)
            is_success_page = true;
        else if (target.find("/payment/fail") == 0)
            is_success_page = false;
        else
            return "";

        std::map<std::string, std::string> params;
        // Находим позицию '?' в URL
        size_t query_pos = target.find('?');
        if (query_pos == std::string::npos) return "";
        std::string query_string = target.substr(query_pos + 1);

        size_t start = 0;
        while (start < query_string.length()) {
            // Находим позицию '='
            size_t eq_pos = query_string.find('=', start);
            if (eq_pos == std::string::npos) break;
            // Находим позицию '&' или конец строки
            size_t amp_pos = query_string.find('&', eq_pos);
            if (amp_pos == std::string::npos) { amp_pos = query_string.length(); }
            // Извлекаем ключ и значение
            std::string key   = query_string.substr(start, eq_pos - start);
            std::string value = query_string.substr(eq_pos + 1, amp_pos - eq_pos - 1);
            // Простой URL decode (заменяем + на пробел)
            for (size_t i = 0; i < value.length(); ++i) {
                if (value[i] == '+') { value[i] = ' '; }
            }
            params[key] = value;
            start       = amp_pos + 1;
        }

        // Извлекаем нужные параметры
        std::string merchant_order_id = params.count("MERCHANT_ORDER_ID") ? params["MERCHANT_ORDER_ID"] : "";
        std::string amount            = amountByMerchant[merchant_order_id]; // Пример - заменить на реальные данные

        auto res = htmlPayPage;
        boost::replace_all(res, "$amount", amount);
        boost::replace_all(res, "$user", merchant_order_id);
        boost::replace_all(res, "$shop", shop_link);
        boost::replace_all(res, "$support", support_link);
        if (is_success_page) boost::replace_all(res, "showPage(false)", "showPage(true)");
        return res;
    }
}