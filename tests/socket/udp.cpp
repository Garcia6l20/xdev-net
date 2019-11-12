#include <net/udp.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <future>
#include <chrono>

#include <net-test.hpp>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

struct UdpTest : testing::NetTest {};

TEST_F(UdpTest, TxRxSequencial) {
    net::udp_socket rx{};
    rx.bind({"127.0.0.1", 4242});

    net::udp_socket{}.send("hello"s, {"localhost", 4242});

    std::string buffer;
    auto from = rx.receive(buffer);
    EXPECT_TRUE(from);
    EXPECT_EQ(buffer, "hello");
    std::cout << from->ip4_address() << " sent: " << buffer << std::endl;
}

TEST_F(UdpTest, TxRxCallback) {
    net::udp_socket rx{};
    rx.bind({"127.0.0.1", 4242});
    using ResutlT = std::tuple<std::string, net::address>;
    auto promise = std::make_shared<std::promise<ResutlT>>();
    auto fut = promise->get_future();

    auto stopper = rx.start_receiver([promise](const std::string& data, const net::address& from) mutable {
        std::cout << data << std::endl;
        promise->set_value({data, from});
    });

    net::udp_socket{}.send("hello"s, {"localhost", 4242});

    auto received = fut.get();
    stopper();

    auto buffer = std::get<0>(received);
    auto from = std::get<1>(received);

    EXPECT_EQ(buffer, "hello");

    std::cout << from.ip4_address() << " sent: " << buffer << std::endl;
}

TEST_F(UdpTest, TxRxFuture) {
    net::udp_socket rx{};
    rx.bind({"127.0.0.1", 4242});
    auto fut = rx.receive<std::string>(500ms);

    net::udp_socket{}.send("hello"s, {"localhost", 4242});

    auto received = fut.get();

    auto buffer = std::get<0>(received);
    auto from = std::get<1>(received);

    EXPECT_EQ(buffer, "hello");

    std::cout << from.ip4_address() << " sent: " << buffer << std::endl;
}
