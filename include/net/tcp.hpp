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
    tcp_client(tcp_client&& other) noexcept: socket{std::move(other)} {}
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
    { a(buffer{}, net::address{}, std::function<void(const net::buffer&)> {}) } -> void;
};
#else
#define ConnectionHandler typename
#define SimpleReceiveFunctor typename
#endif


struct simple_connection_manager {
    using on_receive_func = std::function<void(net::buffer, net::address, std::function<void(const buffer&)>)>;

    simple_connection_manager(net::tcp_client&& client, net::address&& address, std::function<void(void)> delete_notify,
                            on_receive_func on_receive):
        _delete_notify(delete_notify),
        _on_receive(on_receive),
        _client(std::move(client)),
        _address(std::move(address)) {
        std::cout << "client connected: " << _address  << std::endl;
    }
    ~simple_connection_manager() {
        notify_delete();
        stop();
        if (_receive_thread.get_id() != std::this_thread::get_id() && _receive_thread.joinable())
            _receive_thread.join();
        else _receive_thread.detach();
    }
    void message_received(const net::buffer& msg) {
        _on_receive(msg, _address, [this](const net::buffer& data) {
            send(data);
        });
    }
    void start() {
        _receive_thread = _client.start_receiver([this](const auto& data) mutable {
            message_received(data);
        }, [this] {
            disconnected();
        });
    }
    void stop() {
        _receive_thread.request_stop();
    }
    void disconnected() {
        std::cout << "client disconnected: " << _address << std::endl;
        notify_delete();
    }

    void notify_delete() {
        decltype(_delete_notify) notify;
        std::swap(notify, _delete_notify);
        if (notify)
            notify();
    }

    template<DataContainer DataContainerT>
    void send(const DataContainerT& data) const {
        _client.send(data, _address);
    }

protected:
    std::function<void(void)> _delete_notify;
    std::jthread _receive_thread;
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
    [[nodiscard]] std::jthread start_listening(const address& address, int max_conn = 100);
    using on_delete = std::function<void()>;
protected:
    using socket::bind;
    using socket::listen;
    std::map<int, std::shared_ptr<ConnectionHandlerT>> _handlers;
    unique_function<void()> _stop_listen;
    client_handler_creator _connection_create;
    bool _cleaning = false;

    void remove_handler(sock_fd_t fd) {
        if (_cleaning)
            return;
        auto it = _handlers.find(fd);
        if (it != _handlers.end())
            _handlers.erase(it);
    }
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
        auto client_manager = std::make_shared<ConnectionHandlerT>(std::forward<net::socket>(sock),
                                                                   std::forward<net::address>(from),
                                                                   stop_notify,
                                                                   on_receive_func);
        return client_manager;
    }) {
}

template <ConnectionHandler ConnectionHandlerT>
tcp_server<ConnectionHandlerT>::~tcp_server() {
    _cleaning = true;
    for (auto& [_, h]: _handlers)
        h->stop();
    _handlers.clear();
}

template <ConnectionHandler ConnectionHandlerT>
std::jthread tcp_server<ConnectionHandlerT>::start_listening(const address& address, int max_conn) {
    bind(address);
    return listen([this](socket&& socket, net::address&& addr) {
        int fd = socket.fd();
        _handlers.emplace(fd, _connection_create(tcp_client{std::move(socket)}, std::forward<net::address>(addr), [this, fd = socket.fd()]() mutable {
            // deleted
            remove_handler(fd);
        }));
        _handlers.at(fd)->start();
    }, max_conn);
}

}  // namespace xdev::net
