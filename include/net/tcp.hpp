#pragma once

#include <net/socket.hpp>

#ifndef __cpp_unique_function
#include <net/unique_function.hpp>
#else
namespace net {
    template<typename T>
    using unique_function = std::unique_function<T>;
}  // namespace net
#endif

#include <map>

namespace xdev::net {

struct tcp_client: socket
{
    tcp_client();
    tcp_client(socket&& sock): socket{std::move(sock)} {}
    tcp_client(tcp_client&& other): socket{std::move(other)} {}
    ~tcp_client() override;
};

tcp_client::tcp_client():
    socket(family::inet, socket::stream) {
}

tcp_client::~tcp_client() {}

#if defined(__cpp_concepts)
template <typename T>
concept ConnectionHandler = requires(T a) {
    { a.start() } -> void;
    { a.stop() } -> void;
};
template <typename T>
concept SimpleReceiveFunctor = requires(T a) {
    { a(std::string{}, net::address{}) } -> void;
};
#else
#define ConnectionHandler typename
#define SimpleReceiveFunctor typename
#endif


struct simple_connection_manager {
    simple_connection_manager(net::tcp_client&& client, net::address&& address, std::function<void(void)> delete_notify,
                            std::function<void(std::string, net::address)> on_receive):
        _delete_notify(delete_notify),
        _on_receive(on_receive),
        _client(std::move(client)),
        _address(std::move(address)) {
        std::cout << "client connected" << std::endl;
    }
    ~simple_connection_manager() {
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
    using on_receive_func = std::function<void(std::string, net::address)>;
private:
    std::function<void(void)> _delete_notify;
    xdev::thread_guard _receive_guard;
    on_receive_func _on_receive;
    net::tcp_client _client;
    net::address _address;
};

template <ConnectionHandler ConnectionHandlerT = simple_connection_manager>
struct tcp_server: socket
{
    using client_handler_creator = std::function<std::shared_ptr<ConnectionHandlerT>(tcp_client&&, address&&, std::function<void()>)>;
    using stop_notify = std::function<void()>;

    tcp_server(client_handler_creator);

    template <SimpleReceiveFunctor OnReceiveT>
    tcp_server(OnReceiveT on_receive);

    ~tcp_server() override;
    [[nodiscard]] thread_guard start_listening(const address& address, int max_conn = 100);
    using on_delete = std::function<void()>;
private:
    using socket::bind;
    using socket::listen;
    std::map<int, std::shared_ptr<ConnectionHandlerT>> _handlers;
    unique_function<void()> _stop_listen;
    client_handler_creator _connection_create;
    bool _cleaning = false;
};

template <ConnectionHandler ConnectionHandlerT>
tcp_server<ConnectionHandlerT>::tcp_server(client_handler_creator connection_create):
    socket(family::inet, socket::stream),
    _connection_create(connection_create) {
}

template <ConnectionHandler ConnectionHandlerT>
template <SimpleReceiveFunctor OnReceiveT>
tcp_server<ConnectionHandlerT>::tcp_server(OnReceiveT on_receive_func):
    socket(family::inet, socket::stream),
    _connection_create([on_receive_func](net::socket&& sock, net::address&& from, auto stop_notify) {
        auto client_manager = std::make_shared<simple_connection_manager>(std::forward<net::socket>(sock),
                                                                        std::forward<net::address>(from),
                                                                        stop_notify,
                                                                        on_receive_func);
        return client_manager;
    }) {
}

template <ConnectionHandler ConnectionHandlerT>
tcp_server<ConnectionHandlerT>::~tcp_server() {
    for (auto& [_, h]: _handlers)
        h->stop();
    _cleaning = true;
    _handlers.clear();
}

template <ConnectionHandler ConnectionHandlerT>
xdev::thread_guard tcp_server<ConnectionHandlerT>::start_listening(const address& address, int max_conn) {
    bind(address);
    return listen([this](socket&& socket, net::address&& addr) {
        int fd = socket.fd();
        _handlers.emplace(fd, _connection_create(tcp_client{std::move(socket)}, std::forward<net::address>(addr), [this, fd = socket.fd()]() mutable {
            // deleted
            if (_cleaning)
                return;
            auto it = _handlers.find(fd);
            if (it != _handlers.end())
                _handlers.erase(it);
        }));
        _handlers.at(fd)->start();
    }, max_conn);
}

}  // namespace xdev::net
