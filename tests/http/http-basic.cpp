#include <net-test.hpp>

#include <net/http_server.hpp>

#include <iostream>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

struct HttpBasicTest: testing::NetTest {};

struct PrintRequestProcessor {
    net::http_reply operator()(const net::http_request& req) {
        std::cout << req.url << std::endl;
        std::cout << req.status << std::endl;
        for (const auto& [field, value]: req.headers)
            std::cout << field << ": " << value << std::endl;
        switch (req.method) {
        case net::http_method::HTTP_GET:
            std::cout << "GET" << std::endl;
            break;
        case net::http_method::HTTP_POST:
        case net::http_method::HTTP_PUT:
            std::cout << "POST: " << req.body << std::endl;
            break;
        default:
            std::cout << "unhandled method: " << req.method_str() << std::endl;
        }
        return {};
    }
};

TEST_F(HttpBasicTest, Nominal) {

    net::address addr{"localhost", 4242};

    net::http_server<PrintRequestProcessor> srv;
    auto listen_guard = srv.start_listening(addr);

    std::cout << "http server listening at: " << addr << std::endl;

    while(true) {
        std::this_thread::sleep_for(250ms);
    }
}
