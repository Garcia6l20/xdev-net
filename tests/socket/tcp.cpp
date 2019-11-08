#include <net/tcp.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <cassert>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;

struct base_connection_manager {
    base_connection_manager(net::tcp_client&& client, net::address&& address, std::function<void(void)> on_delete):
        _on_delete(on_delete),
        _client(std::move(client)),
        _address(std::move(address)){
        std::cout << "client connected" << std::endl;
    }
    base_connection_manager(base_connection_manager&& other):
        _on_delete(std::move(other._on_delete)),
        _client(std::move(other._client)),
        _address(std::move(other._address)){
        other._on_delete = nullptr;
    }
    virtual ~base_connection_manager() {
        stop();
        if (_on_delete)
            _on_delete();
        _on_delete = nullptr;
    }
    virtual void message_received(const std::string& msg) {
        std::cout << "data received" << std::endl;
    }
    void start() {
        _receive_stopper = _client.start_receiver([this](const std::string& data, const net::address& /*from*/) mutable {
            message_received(data);
        });
    }
    void stop() {
        if (_receive_stopper)
            _receive_stopper();
        _receive_stopper = nullptr;
    }
private:
    std::function<void(void)> _on_delete;
    net::thread_stopper _receive_stopper;
    net::tcp_client _client;
    net::address _address;
};

TEST(TCP, TxRxSequencial) {

    net::tcp_server<base_connection_manager> srv{};
    srv.start_listening({"localhost", 4242});

    std::this_thread::sleep_for(1s);

    net::tcp_client clt{};
    clt.connect({"localhost", 4242});
    clt.send("hello"s, {"localhost", 4242});

    std::this_thread::sleep_for(500ms);
}
