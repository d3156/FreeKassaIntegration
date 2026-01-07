#include "FreeKassa.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <ostream>
#include <string>

using namespace boost;

int main(int argc, char *argv[])
{
    std::string paymentId = getenv("ORDER_ID") ? getenv("ORDER_ID") : "Bob";
    int amount            = atoi(getenv("AMOUNT") ? getenv("AMOUNT") : "2000");
    for (int i = 0; i < argc; ++i) { printf("parameter %d: \"%s\"\n", i, argv[i]); }

    std::locale::global(std::locale("en_US.UTF-8"));
    try {
        boost::asio::io_context io;
        auto guard = boost::asio::make_work_guard(io);

        FreeKassa::Client freekassaClient(io);

        std::cout << freekassaClient.createFreeKassaLink(amount, paymentId) << std::endl;

        io.run();
    } catch (std::exception e) {
        std::cout << "[Abort Error]" << e.what();
    }
    return 0;
}