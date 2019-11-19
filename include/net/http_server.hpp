#pragma once

#include <net/tcp.hpp>
#include <net/http_request.hpp>
#include <net/http_reply.hpp>
#include <net/router.hpp>

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
    { a.body } -> DataContainer;
};
template <typename T>
concept StreamBodyProvider = requires(T a) {
    { a.body() }-> std::istream;
};
template <typename T>
concept ChunkedBodyProvider = requires(T a) {
    { a.body() }-> std::optional<buffer>;
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

struct simple_http_connection_handler: simple_connection_manager {
    using base = simple_connection_manager;
    using base::base; // forward ctors

    std::optional<http_request_parser> _parser;

    struct context_t {
        http_request request;
        void send_reply(HttpReplyProvider&& reply) {
            _handler.send(std::forward<decltype(reply)>(reply));
        }
        template <typename T, typename...Args>
        void make_userdata(Args&&...args) {
            _userdata = std::make_shared<T>(std::forward<Args>(args)...);
        }
        template <typename T>
        std::shared_ptr<T> userdata() {
            return std::dynamic_pointer_cast<T>(_userdata);
        }
    private:
        std::shared_ptr<void> _userdata;
        context_t(simple_http_connection_handler& handler): _handler{handler} {}
        simple_http_connection_handler& _handler;
        friend struct simple_http_connection_handler;
    };
    context_t _context {*this};

    using router_t = base_router<void, context_t>;
    std::shared_ptr<router_t> _router;

    void send(HttpReplyProvider&& reply) const {
        base::send(reply.http_head());
        overloaded {
            [](auto&&) {throw std::runtime_error("Unhandled HttpReplyProvider");},
            [this](StreamBodyProvider&&provider) {
                std::istream body = provider.body();
                buffer buf(4096);
                while (!body.eof()) {
                    body.read(buf.data(), buf.size());
                    auto bytes = body.gcount();
                    if (bytes == buf.size()) {
                        base::send(buf);
                    } else {
                        buf.resize(bytes);
                        base::send(buf);
                    }
                }
            },
            [this](BasicBodyProvider&&provider) {
                base::send(provider.body);
            }
        } (reply);
    }

    void set_router(std::shared_ptr<router_t>& router) {
        _router = router;
    }

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
                _context.request = http_request::build(parser);
                try {
                    (*_router)(_context.request.path, _context);
                } catch (const router_t::not_found&) {
                    http_reply rep;
                    rep.status = http_status::HTTP_STATUS_NOT_FOUND;
                    rep.body = {"not found"};
                    send(rep);
                } catch (const std::exception& err) {
                    http_reply rep;
                    rep.status = http_status::HTTP_STATUS_INTERNAL_SERVER_ERROR;
                    rep.body = {err.what()};
                    send(rep);
                } catch (...) {
                    http_reply rep;
                    rep.status = http_status::HTTP_STATUS_INTERNAL_SERVER_ERROR;
                    rep.body = {"unknown error"};
                    send(rep);
                }

                _parser.reset();
            }
        }, [this] {
            disconnected();
        });
    }
};

struct http_server: tcp_server<simple_http_connection_handler>
{
    using base = tcp_server<simple_http_connection_handler>;
    using context = simple_http_connection_handler::context_t;
    inline http_server();

    template <typename ViewHandlerT>
    void operator()(const std::string&path, ViewHandlerT handler) {
        _router->add_route(path, handler);
    }

    std::jthread start_listening(const address& address, int max_conn = 5) {
        bind(address);
        return listen([this](socket&& socket, net::address&& addr) {
            int fd = socket.fd();
            auto handler = _connection_create(tcp_client{std::move(socket)}, std::forward<net::address>(addr), [this, fd = socket.fd()]() mutable {
                remove_handler(fd);
            });
            handler->set_router(_router);
            _handlers.emplace(fd, handler);
            _handlers.at(fd)->start();
        }, max_conn);
    }

private:
    std::shared_ptr<simple_http_connection_handler::router_t> _router = std::make_shared<simple_http_connection_handler::router_t>();
};

http_server::http_server(): base([](buffer&&, net::address&&, auto&&) {}) {}

}  // namespace xdev::net
