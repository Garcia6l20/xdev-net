#pragma once

#include <net/tcp.hpp>

namespace xdev::net {

namespace node {
#include <http_parser.h>
}

struct http_connection_handler: simple_connection_manager {
    using base = simple_connection_manager;
    using base::base; // forward ctors

    void start() {
        http_parser_init(&_parser, node::HTTP_REQUEST);
        _parser.data = this;
        _receive_guard = _client.start_receiver([this](const std::string& data, const net::address& /*from*/) mutable {
            auto res = http_parser_execute(&_parser, &_parser_settings, data.data(), data.length());
            if (res < data.length()) {
                throw error("http parser error: " +
                            std::string(http_errno_description(node::http_errno(_parser.http_errno))));
                // stop & notify server we stopped
                stop();
                if (_delete_notify)
                    _delete_notify();
                _delete_notify = nullptr;
            } else {

            }
        });
    }

private:

    static http_connection_handler* manager(node::http_parser* p) {
        return static_cast<http_connection_handler*>(p->data);
    }

    static int _on_msg_begin(node::http_parser* p) {
        return 0;
    }
    static int _on_url(node::http_parser* p, const char *at, size_t length) {
        return 0;
    }
    static int _on_status(node::http_parser* p, const char *at, size_t length) {
        return 0;
    }
    static int _on_header_field(node::http_parser* p, const char *at, size_t length) {
        return 0;
    }
    static int _on_header_value(node::http_parser* p, const char *at, size_t length) {
        return 0;
    }
    static int _on_headers_complete(node::http_parser* p) {
        return 0;
    }
    static int _on_body(node::http_parser* p, const char *at, size_t length) {
        return 0;
    }
    static int _on_msg_complete(node::http_parser* p) {
        return 0;
    }
    static int _on_chunk_header(node::http_parser* p) {
        return 0;
    }
    static int _on_chunk_complete(node::http_parser* p) {
        return 0;
    }

    node::http_parser _parser;
    node::http_parser_settings _parser_settings = {
        &_on_msg_begin,
        &_on_url,
        &_on_status,
        &_on_header_field,
        &_on_header_value,
        &_on_headers_complete,
        &_on_body,
        &_on_msg_complete,
        &_on_chunk_header,
        &_on_chunk_complete
    };
};

template <ConnectionHandler ConnectionHandlerT = http_connection_handler>
struct http_server: tcp_server<ConnectionHandlerT>
{
    using base = tcp_server<ConnectionHandlerT>;
    http_server();
private:
};

template <ConnectionHandler ConnectionHandlerT>
http_server<ConnectionHandlerT>::http_server(): base([](std::string&&, net::address&&, auto&&) {}) {}

}  // namespace xdev::net
