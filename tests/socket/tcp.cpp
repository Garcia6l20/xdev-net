#include <net/tcp.hpp>

#include <net-test.hpp>

#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <cassert>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

struct test_connection_manager {
    test_connection_manager(net::tcp_client&& client, net::address&& address, std::function<void(void)> delete_notify,
                            std::function<void(std::string, net::address)> on_receive):
        _delete_notify(delete_notify),
        _on_receive(on_receive),
        _client(std::move(client)),
        _address(std::move(address)) {
        std::cout << "client connected" << std::endl;
    }
    ~test_connection_manager() {
        stop();
        if (_delete_notify)
            _delete_notify();
        _delete_notify = nullptr;
    }
    void message_received(const std::string& msg) {
        _on_receive(msg, _address);
    }
    void start() {
        _receive_guard = _client.start_receiver([this](const std::string& data, const net::address& /*from*/) mutable {
            message_received(data);
        });
    }
    void stop() {
        _receive_guard();
    }
private:
    std::function<void(void)> _delete_notify;
    xdev::thread_guard _receive_guard;
    std::function<void(std::string, net::address)> _on_receive;
    net::tcp_client _client;
    net::address _address;
};

struct TcpTest : testing::NetTest {};

TEST_F(TcpTest, SimpleConnectionManager) {
    std::promise<std::tuple<std::string, net::address>> promise;
    auto fut = promise.get_future();

    auto on_data = [&promise](const std::string& data, const net::address& from, const auto& send) {
        promise.set_value({data, from});
        send({data.begin(), data.end()});
    };

    net::tcp_server srv{on_data};

    net::address srv_addr{"localhost", 4242};

    auto stopper = srv.start_listening(srv_addr);

    std::this_thread::sleep_for(10ms);

    net::tcp_client clt{};
    clt.connect({"localhost", 4242});
    clt.send("hello"s, {"localhost", 4242});

    auto [received, from] = fut.get();
    std::cout << from << " sent: " << received << std::endl;
    ASSERT_EQ("hello", received);

    std::this_thread::sleep_for(10ms);

    std::string back;
    auto from_srv = clt.receive(back);
    std::cout << srv_addr << " responded: " << back << std::endl;
    ASSERT_TRUE(from_srv);
    ASSERT_EQ(received, back);
    stopper();
}

TEST_F(TcpTest, SimpleConnectionManagerFutureReceiveBack) {
    std::promise<std::tuple<std::string, net::address>> promise;
    auto fut = promise.get_future();

    auto on_data = [&promise](const std::string& data, const net::address& from, const auto& send) {
        promise.set_value({data, from});
        send({data.begin(), data.end()});
    };

    net::tcp_server srv{on_data};

    auto stopper = srv.start_listening({"localhost", 4242});

    std::this_thread::sleep_for(10ms);

    net::tcp_client clt{};
    clt.connect({"localhost", 4242});
    clt.send("hello"s, {"localhost", 4242});

    auto [received, from] = fut.get();
    std::cout << from << " sent: " << received << std::endl;
    ASSERT_EQ("hello", received);

    auto clt_fut = clt.receive<std::string>(10ms);
    auto received_back = clt_fut.get();
    ASSERT_EQ(received, std::get<0>(received_back));
    stopper();
}

TEST_F(TcpTest, CustomConnectionManager) {

    std::promise<std::tuple<std::string, net::address>> promise;
    auto fut = promise.get_future();

    auto on_data = [&promise](const std::string& data, const net::address& from) {
        promise.set_value({data, from});
    };

    net::tcp_server<test_connection_manager> srv{[on_data](net::socket&& sock, net::address&& from, auto stop_notify) {
        auto client_manager = std::make_shared<test_connection_manager>(std::forward<net::socket>(sock),
                                                                        std::forward<net::address>(from),
                                                                        stop_notify,
                                                                        on_data);
        return client_manager;
    }};

    auto stopper = srv.start_listening({"localhost", 4242});

    net::tcp_client clt{};
    clt.connect({"localhost", 4242});
    clt.send("hello"s, {"localhost", 4242});

    auto [received, from] = fut.get();
    std::cout << from << " sent: " << received << std::endl;
    ASSERT_EQ("hello", received);
    stopper();
}
