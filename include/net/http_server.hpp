#pragma once

#include <net/tcp.hpp>
#include <net/http_request.hpp>

namespace xdev::net {

template<typename CharT, typename TraitsT = std::char_traits<CharT> >
class vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT> {
public:
    vectorwrapbuf(std::vector<CharT>& vec) {
        setg(vec.data(), vec.data(), vec.data() + vec.size());
    }
};

struct buffer_istreambuf: std::basic_streambuf<char> {
    buffer_istreambuf(buffer& buf) {
        setg(buf.data(), buf.data(), buf.data() + buf.size());
    }
};

#if defined(__cpp_concepts) || defined(_MSC_VER)
template <typename T>
concept HttpHeadProvider = requires(T a) {
    { a.http_head() }->std::string;
};
template <typename T>
concept BasicBodyProvider = requires(T a) {
    { a.body() }-> DataContainer;
};
template <typename T>
concept StreamBodyProvider = requires(T a) {
    { a.body() }-> std::istream;
};
template <typename T>
concept ChunkedBodyProvider = requires(T a) {
    { a.next_body() }-> std::optional<buffer>;
};
template <typename T>
concept HttpReplyProvider = HttpHeadProvider<T> && ( StreamBodyProvider<T> || ChunkedBodyProvider<T> || BasicBodyProvider<T>);

template <typename T>
concept HttpRequestLisenerTrait = requires(T a) {
    {a(http_request{})} -> HttpReplyProvider;
};
#else
#define BodyProvider typename
#define HttpRequestLisenerTrait typename
#define HttpHeadProvider typename
#define BasicBodyProvider typename
#define StreamBodyProvider typename
#define ChunkedBodyProvider typename
#define HttpReplyProvider typename
#endif

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
                send(reply.http_head());

                using ReplyType = decltype(reply);
                overloaded {
                    [](auto&&) {throw std::runtime_error("Unhandled ReplyType");},
                    [this](StreamBodyProvider&&provider) {
                        std::istream body = provider.body();
                        buffer buf(4096);
                        while (!body.eof()) {
                            body.read(buf.data(), buf.size());
                            auto bytes = body.gcount();
                            if (bytes == buf.size()) {
                                send(buf);
                            } else {
                                buf.resize(bytes);
                                send(buf);
                            }
                        }
                    },
                    [this](BasicBodyProvider&&provider) {
                        send(provider.body());
                    }
                } (reply);
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
