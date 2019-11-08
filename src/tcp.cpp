#include <net/tcp.hpp>

using namespace net;

tcp_client::tcp_client():
    socket(family::inet, socket::stream) {
}

tcp_client::~tcp_client() {}


tcp_server::tcp_server(client_handler_creator connection_create):
    socket(family::inet, socket::stream),
    _connection_create(connection_create) {
}

tcp_server::~tcp_server() {
    for (auto& [_, h]: _handlers)
        h->stop();
    _cleaning = true;
    _handlers.clear();
}

thread_stopper tcp_server::start_listening(const address& address, int max_conn) {
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

abstract_tcp_client_handler::~abstract_tcp_client_handler() {}
