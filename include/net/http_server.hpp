#pragma once

#include <net/tcp.hpp>
#include <net/http_request.hpp>

#include <sstream>
#include <ostream>
#include <iterator>

namespace xdev::net {

using http_status = node::http_status;
const auto http_status_str = node::http_status_str;

struct http_reply {

    http_reply() = default;
    http_reply(std::string_view body) {
        _body.assign(body.begin(), body.end());
    }

    http_status _status = http_status::HTTP_STATUS_OK;
    http_headers _headers = { {{"Content-Type"}, {"application/x-empty"}} };
    buffer _body;

    std::tuple<std::string, buffer> operator()() {
        _headers["Content-Length"] = std::to_string(_body.size());
        std::ostringstream ss;
        ss << "HTTP/1.1 " << _status << " " << http_status_str(_status) << "\r\n";
        for (auto&& [field, value] : _headers)
            ss << field << ": " << value << "\r\n";
        ss << "\r\n";
        auto http_head = ss.str();
        return { ss.str(), std::move(_body) };
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
                auto [http, content] = RequestListenerT{}(std::move(http_request::build(parser)))();
                send(http);
                send(content);
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
