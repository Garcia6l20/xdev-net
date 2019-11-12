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
#else
#define ConnectionHandler typename
#endif

template <ConnectionHandler ConnectionHandlerT>
struct tcp_server: socket
{
    using client_handler_creator = std::function<std::shared_ptr<ConnectionHandlerT>(tcp_client&&, address&&, std::function<void()>)>;
    using stop_notify = std::function<void()>;

    tcp_server(client_handler_creator);
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
