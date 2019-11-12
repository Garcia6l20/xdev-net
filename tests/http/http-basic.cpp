#include <net-test.hpp>

#include <net/http_server.hpp>

#include <iostream>

struct HttpBasicTest: testing::NetTest {};

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

TEST_F(HttpBasicTest, Nominal) {

    net::address addr{"localhost", 4242};

    net::http_server srv;
    auto listen_guard = srv.start_listening(addr);

    std::cout << "http server listening at: " << addr << std::endl;

    while(true) {
        std::this_thread::sleep_for(250ms);
    }
}
