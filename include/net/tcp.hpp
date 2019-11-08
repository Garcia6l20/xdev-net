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

namespace net {

struct tcp_client: socket
{
    tcp_client();
    tcp_client(socket&& sock): socket{std::move(sock)} {}
    tcp_client(tcp_client&& other): socket{std::move(other)} {}
    ~tcp_client() override;
};

#if defined(__cpp_concepts)
template <typename T>
concept ConnectionHandler = requires(T a) {
    { T(net::tcp_client{}, net::address{}, std::function<void()> {}) } -> T;
    { a.start() } -> void;
    { a.stop() } -> void;
};
#else
#define ConnectionHandler typename
#endif

template <ConnectionHandler ConnectionHandlerT>
struct tcp_server: socket
{
    tcp_server();
    ~tcp_server() override;
    auto start_listening(const address& address, int max_conn = 100);
    void stop_listen() {
        if (_stop_listen)
            _stop_listen();
        _stop_listen = nullptr;
    }

    using on_delete = std::function<void()>;
private:
    using socket::bind;
    using socket::listen;
    unique_function<void()> _stop_listen;
    std::map<int, ConnectionHandlerT> _handlers;
    bool _cleaning = false;
};

template <ConnectionHandler ConnectionHandlerT>
tcp_server<ConnectionHandlerT>::tcp_server():
    socket(family::inet, socket::stream) {
}

template <ConnectionHandler ConnectionHandlerT>
tcp_server<ConnectionHandlerT>::~tcp_server() {
    stop_listen();
    for (auto& [_, h]: _handlers)
        h.stop();
    _cleaning = true;
    _handlers.clear();
}

template <ConnectionHandler ConnectionHandlerT>
auto tcp_server<ConnectionHandlerT>::start_listening(const address& address, int max_conn) {
    bind(address);
    _stop_listen = listen([this](socket&& socket, net::address&& addr) {
        int fd = socket.fd();
        _handlers.emplace(fd, ConnectionHandlerT(tcp_client{std::move(socket)}, std::forward<net::address>(addr), [this, fd = socket.fd()]() mutable {
            // deleted
            if (_cleaning)
                return;
            auto it = _handlers.find(fd);
            if (it != _handlers.end())
                _handlers.erase(it);
        }));
        _handlers.at(fd).start();
    }, max_conn);
    return [this] {
        stop_listen();
    };
}

}  // namespace net
