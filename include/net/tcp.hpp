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

struct abstract_tcp_client_handler {
    virtual ~abstract_tcp_client_handler();
    virtual void start() = 0;
    virtual void stop() = 0;
};

struct tcp_server: socket
{
    using client_handler_creator = std::function<std::shared_ptr<abstract_tcp_client_handler>(tcp_client&&, address&&, std::function<void()>)>;
    using stop_notify = std::function<void()>;

    tcp_server(client_handler_creator);
    ~tcp_server() override;
    [[nodiscard]] thread_guard start_listening(const address& address, int max_conn = 100);
    using on_delete = std::function<void()>;
private:
    using socket::bind;
    using socket::listen;
    std::map<int, std::shared_ptr<abstract_tcp_client_handler>> _handlers;
    unique_function<void()> _stop_listen;
    client_handler_creator _connection_create;
    bool _cleaning = false;
};

}  // namespace xdev::net
