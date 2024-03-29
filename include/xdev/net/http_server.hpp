#pragma once

#include <xdev/net.hpp>

#include <xdev/net/router.hpp>
#include <xdev/net/config.hpp>

#include <boost/asio/spawn.hpp>

#include <any>
#include <functional>
#include <variant>
#include <iostream>

namespace xdev::net {

namespace http {

template <class Derived, typename...BodyTypes>
class session {
public:

    using body_traits = details::body_traits<string_body, file_body, dynamic_body, BodyTypes...>;

    struct context {
        template <typename BodyType = string_body>
        const request<BodyType>& req() const {
            return std::get<request<BodyType>>(_request_var);
        }
        template <typename BodyType = string_body>
        request<BodyType>& req() {
            return std::get<request<BodyType>>(_request_var);
        }
        template <typename BodyType = string_body, typename...Args>
        response<BodyType> make_response(Args&&...args) {
            return std::visit([...args = std::forward<Args>(args)](auto&&request) {
                if constexpr (sizeof...(args) == 0) {
                    return net::http::response<BodyType> {
                        http::status::ok, request.version()
                    };
                } else {
                    net::http::response<BodyType> response {
                        std::forward<Args>(args)...
                    };
                    response.version(request.version());
                    return std::move(response);
                }
            }, _request_var);
        }
        template <typename T, typename...Args>
        T& make_data(Args&&...args) {
            _data = std::make_shared<T>(std::forward<Args>(args)...);
            return data<T>();
        }
        template <typename T>
        T& data() {
            return *std::static_pointer_cast<T>(_data).get();
        }
    private:
        typename body_traits::request_variant _request_var;
        std::shared_ptr<void> _data;
        friend class session<Derived, BodyTypes...>;
    };

    using router_type = base_router<context, string_body, file_body, dynamic_body, BodyTypes...>;
    using router_ptr = std::shared_ptr<router_type>;

    bool _close;

    session(router_ptr router):
        _router{router} {
    }

    template<bool isRequest, class Body, class Fields>
    bool send(beast::http::message<isRequest, Body, Fields>&& msg, asio::yield_context yield) {
        _close = msg.need_eof();
        serializer<isRequest, Body, Fields> sr{msg};
        async_write(derived().stream(), sr, yield);
        return msg.need_eof();
    }

    bool send(status status, asio::yield_context yield) {
        return send(response<empty_body> {
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(status, XDEV_NET_HTTP_DEFAULT_VERSION)
        }, yield);
    }

    bool send(status status, std::string_view message, asio::yield_context yield) {
        return send(response<string_body> {
            std::piecewise_construct,
            std::make_tuple(message),
            std::make_tuple(status, XDEV_NET_HTTP_DEFAULT_VERSION)
        }, yield);
    }

    void read(asio::yield_context yield) {
        beast::error_code ec;
        beast::flat_buffer buffer;
        context ctx;
        std::string chunk;
        chunk_extensions ce;
        auto chunk_header_cb = [&](std::uint64_t size, boost::string_view extensions, error_code&ec) mutable {
            ce.parse(extensions, ec);
            if (ec)
                return;
            if (size > std::numeric_limits<std::size_t>::max()) {
                ec = error::body_limit;
            }
            chunk.reserve(size);
            chunk.clear();
        };
        auto chunk_body_cb = [&](std::uint64_t remain, boost::string_view body, error_code&ec) mutable {
            if (remain == body.size())
                ec = error::end_of_chunk;
            chunk.append(body.data(), body.size());
            return body.size();
        };
        typename router_type::route* current_route = nullptr;
        std::smatch current_match;
        typename body_traits::parser_variant parser_var;
        for (;;) {
            ce.clear();

            request_parser<empty_body> req0;
            req0.on_chunk_header(chunk_header_cb);
            req0.on_chunk_body(chunk_body_cb);
            async_read_header(derived().stream(), buffer, req0, yield[ec]);
            // get associated route
            try {
                auto target = req0.get().target();
                if (!target.empty()) {
                    std::string path{target.data(), target.size()};
                    auto route_data = _router->route_for(path);
                    current_route = &std::get<0>(route_data);
                    current_match = std::move(std::get<1>(route_data));
                    current_route->do_init(current_match, ctx, parser_var, req0);
                    std::visit([&chunk_header_cb, &chunk_body_cb](auto&p) mutable {
                        p.on_chunk_header(chunk_header_cb);
                        p.on_chunk_body(chunk_body_cb);
                    }, parser_var);
                    while (!std::visit([](auto& p) {
                        return p.is_done();
                    }, parser_var)) {
                        std::visit([this, yield, &ec, &buffer, &ctx, &ce, &current_match, &current_route, &chunk](auto&p) mutable {
                            async_read_some(derived().stream(), buffer, p, yield[ec]);
                            if(ec && ec != error::end_of_chunk) {
                                throw std::runtime_error(ec.message());
                            } else
                                ec.assign(0, ec.category());
                            // We got a whole chunk, print the extensions:
                            for(auto const& extension : ce)
                            {
                                std::cout << "Extension: " << std::get<0>(extension);
                                if(! std::get<1>(extension).empty())
                                    std::cout << " = " << std::get<1>(extension) << std::endl;
                                else
                                    std::cout << std::endl;
                            }
                            if (p.chunked() && chunk.size()) {
                                if (!current_route->do_chunk(chunk, current_match, ctx)) {
                                    send(status::not_implemented, yield[ec]);
                                }
                            }
                        }, parser_var);
                    }
                    std::visit([&ctx] (auto& p) {
                        ctx._request_var = p.release();
                    }, parser_var);
                    if (ec == http::error::end_of_stream)
                        break;
                    if  (ec)
                        throw std::runtime_error(ec.message());
                    std::visit([this, yield, &ec, &ctx](auto&&resp) {
                        std::visit([&resp](auto&request) {
                            resp.version(request.version());
                            resp.keep_alive(request.keep_alive());
                        }, ctx._request_var);
                        send(std::move(resp), yield);
                    }, current_route->do_complete(current_match, ctx));
                } else {
                    throw std::runtime_error("bug");
                }
            } catch(const typename router_type::not_found&) {
                send(net::http::status::not_found, yield[ec]);
                _close = true;
            } catch(const std::exception& err) {
                send(net::http::status::internal_server_error, err.what(), yield[ec]);
                _close = true;
            }
            if (_close || ec)
                break;
        }
        derived().do_eof();
    }

private:

    Derived&
    derived()
    {
        return static_cast<Derived&>(*this);
    }

    router_ptr _router;
};

template <typename...BodyTypes>
class plain_session: public session<plain_session<BodyTypes...>, BodyTypes...>,
                     public std::enable_shared_from_this<plain_session<BodyTypes...>>
{
public:
    using router_type = typename session<plain_session<BodyTypes...>, BodyTypes...>::router_type;
    using router_ptr = typename session<plain_session<BodyTypes...>, BodyTypes...>::router_ptr;

    plain_session(tcp::socket&& socket, router_ptr router):
        session<plain_session<BodyTypes...>, BodyTypes...> (router),
        _stream(std::move(socket)) {}

    void run(asio::yield_context yield) {
        auto self{this->shared_from_this()};
        this->read(yield);
    }

    beast::tcp_stream&
    stream() {
        return _stream;
    }

    void
    do_eof()
    {
        // Send a TCP shutdown
        error_code ec;
        _stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
private:
    beast::tcp_stream _stream;
};

template <typename...BodyTypes>
class ssl_session: public session<ssl_session<BodyTypes...>, BodyTypes...>,
                   public std::enable_shared_from_this<ssl_session<BodyTypes...>>
{
public:
    using router_type = typename session<ssl_session<BodyTypes...>, BodyTypes...>::router_type;
    using router_ptr = typename session<ssl_session<BodyTypes...>, BodyTypes...>::router_ptr;

    ssl_session(tcp::socket&& socket, router_ptr router, ssl::context& ctx):
        session<ssl_session<BodyTypes...>, BodyTypes...> (router),
        _stream(std::move(socket), ctx) {}

    void run(asio::yield_context yield) {
        auto self{this->shared_from_this()};
        error_code ec;
        _stream.async_handshake(ssl::stream_base::server, yield[ec]);
        if (ec)
            return;
        this->read(yield);
    }

    beast::ssl_stream<beast::tcp_stream>&
    stream() {
        return _stream;
    }

    void
    do_eof()
    {
        // Send a TCP shutdown
        error_code ec;
        beast::get_lowest_layer(_stream).socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
private:
    beast::ssl_stream<beast::tcp_stream> _stream;
};

template <bool ssl, typename...BodyTypes>
struct session_wrap;

template <typename...BodyTypes>
struct session_wrap<true, BodyTypes...> {
    using type = ssl_session<BodyTypes...>;
};

template <typename...BodyTypes>
struct session_wrap<false, BodyTypes...> {
    using type = plain_session<BodyTypes...>;
};

template <bool _use_ssl, typename...BodyTypes>
class server {
public:
    using session_type = typename session_wrap<_use_ssl, BodyTypes...>::type;
    using router_type = typename session_type::router_type;
    using context_type = typename session_type::context;
    using route_return_type = typename router_type::return_type;
    using route_init_return_type = typename router_type::init_return_type;

    server(asio::io_context& ctx, const tcp::endpoint& endpoint):
        _acceptor{ctx, endpoint},
        _socket{ctx},
        _ssl_context{nullptr} {
        asio::spawn(ctx, std::bind(&server::_accept, this, std::placeholders::_1));
    }

    server(asio::io_context& ctx, const tcp::endpoint& endpoint, ssl::context& ssl_context) requires(_use_ssl):
        server(ctx, endpoint) { _ssl_context = &ssl_context; }

    auto& on(const std::string&path) {
        return _router->add_route(path);
    }

private:
    void _accept(asio::yield_context yield) {
        error_code ec;
        for (;;) {
            _acceptor.async_accept(_socket, yield[ec]);
            if (!ec) {
                std::shared_ptr<session_type> sess;
                if constexpr (_use_ssl) {
                    if (_ssl_context == nullptr) {
                        throw std::runtime_error("no ssl context");
                    }
                    sess = std::make_shared<session_type>(std::move(_socket),
                                                          _router,
                                                          *_ssl_context);
                } else {
                    sess = std::make_shared<session_type>(std::move(_socket),
                                                          _router);
                }
                asio::spawn(_acceptor.get_executor(),
                            [sess] (asio::yield_context yield) {
                    sess->run(yield);
                });
            }
        }
    }

    ssl::context* _ssl_context;
    tcp::acceptor _acceptor;
    tcp::socket _socket;
    typename session_type::router_ptr _router = std::make_shared<typename session_type::router_type>();
};

using plain_server = server<false>;
using ssl_server = server<true>;

} // namespace http

}  // namespace xdev::net
