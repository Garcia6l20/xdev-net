#pragma once

#include <net/tcp.hpp>
#include <net/http_request.hpp>

namespace xdev::net {

struct http_reply {
    std::string operator()() {
        return "";
    }
};

#if defined(__cpp_concepts)
template <typename T>
concept HttpRequestLisenerTrait = requires(T a) {
    {a(http_request{})} -> http_reply;
};
#else
#define HttpRequestLisenerTrait typename
#endif

template <HttpRequestLisenerTrait RequestListenerT>
struct simple_http_connection_handler: simple_connection_manager {
    using base = simple_connection_manager;
    using base::base; // forward ctors

    std::optional<http_request_parser> _parser;

    void start() {
        _receive_thread = _client.start_receiver([this](const buffer& data, const net::address& /*from*/) mutable {
            if (!_parser)
                _parser.emplace();
            auto& parser = _parser.value();
            if (!parser(data)) {
                // stop & notify server we stopped
                stop();
                disconnected();

                throw error("http parser error: " + _parser.value().error_description());
            } else if (parser) {
                // message complete
                auto reply = RequestListenerT{}(std::move(http_request::build(parser)));
                send(reply());
                _parser.reset();
            }
        }, [this] {
            disconnected();
        });
    }

private:
};

template <HttpRequestLisenerTrait RequestListenerT>
struct http_server: tcp_server<simple_http_connection_handler<RequestListenerT>>
{
    using base = tcp_server<simple_http_connection_handler<RequestListenerT>>;
    http_server();
private:
};

template <HttpRequestLisenerTrait RequestListenerT>
http_server<RequestListenerT>::http_server(): base([](buffer&&, net::address&&, auto&&) {}) {}

}  // namespace xdev::net
