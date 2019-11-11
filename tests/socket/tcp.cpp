#include <net/tcp.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <cassert>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;

struct test_connection_manager: net::abstract_tcp_client_handler {
    test_connection_manager(net::tcp_client&& client, net::address&& address, std::function<void(void)> delete_notify,
                            std::function<void(std::string, net::address)> on_receive):
        _delete_notify(delete_notify),
        _on_receive(on_receive),
        _client(std::move(client)),
        _address(std::move(address)) {
        std::cout << "client connected" << std::endl;
    }
    ~test_connection_manager() override {
        stop();
        if (_delete_notify)
            _delete_notify();
        _delete_notify = nullptr;
    }
    void message_received(const std::string& msg) {
        _on_receive(msg, _address);
    }
    void start() override {
        _receive_guard = std::move(_client.start_receiver([this](const std::string& data, const net::address& /*from*/) mutable {
            message_received(data);
        }));
    }
    void stop() override {
        _receive_guard();
    }
private:
    std::function<void(void)> _delete_notify;
    net::thread_guard _receive_guard;
    std::function<void(std::string, net::address)> _on_receive;
    net::tcp_client _client;
    net::address _address;
};

struct TcpTest : testing::Test {
protected:
	void SetUp() override;
	void TearDown() override;
};

void TcpTest::SetUp() {
	net::initialize();
	// Initialize Winsock
	WSADATA wsaData;
	if (int res = WSAStartup(MAKEWORD(2, 2), &wsaData); res) {
		throw std::string("WSAStartup failed: " + std::to_string(res));
	}
}

void TcpTest::TearDown() {
	WSACleanup();
	net::cleanup();
}

TEST_F(TcpTest, TxRxSequencial) {

    std::promise<std::tuple<std::string, net::address>> promise;
    auto fut = promise.get_future();

    auto on_data = [&promise](const std::string& data, const net::address& from) {
        promise.set_value({data, from});
    };

    net::tcp_server srv{[on_data](net::socket&& sock, net::address&& from, net::tcp_server::stop_notify stop_notify) {
        auto client_manager = std::make_shared<test_connection_manager>(std::forward<net::socket>(sock),
                                                                        std::forward<net::address>(from),
                                                                        stop_notify,
                                                                        on_data);
        return client_manager;
    }};

    auto stopper = srv.start_listening({"localhost", 4242});

    std::this_thread::sleep_for(1s);

    net::tcp_client clt{};
    clt.connect({"localhost", 4242});
    clt.send("hello"s, {"localhost", 4242});

    auto [received, from] = fut.get();
    std::cout << from.ip4_address() << " sent: " << received << std::endl;
    ASSERT_EQ("hello", received);

    stopper();
}
