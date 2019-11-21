#pragma once

#include <net/net.hpp>
#include <net/router.hpp>

#include <boost/asio/spawn.hpp>

#include <functional>
#include <variant>

namespace xdev::net {

namespace http {

template <typename...BodyTypes>
class session: public std::enable_shared_from_this<session<BodyTypes...>> {
public:

    using router_return_type = std::variant<response<string_body>,
                                            response<file_body>,
                                            response<dynamic_body>,
                                            response<BodyTypes>...>;
    using router_parser_variant = std::variant<request_parser<string_body>,
                                               request_parser<file_body>,
                                               request_parser<dynamic_body>,
                                               request_parser<BodyTypes>...>;
    using request_variant = std::variant<request<string_body>,
                                         request<file_body>,
                                         request<dynamic_body>,
                                         request<BodyTypes>...>;

    struct context {
        template <typename BodyType = string_body>
        request<BodyType> req() {
            return std::get<request<BodyType>>(_request_var);
        }
    private:
        request_variant _request_var;
        friend class session<BodyTypes...>;
    };

    using router_type = base_router<router_return_type, context, router_parser_variant>;
    using router_ptr = std::shared_ptr<router_type>;

    bool _close;

    session(tcp::socket socket, router_ptr router):
        _stream{std::move(socket)}, _router{router} {
    }

    template<bool isRequest, class Body, class Fields>
    bool send(beast::http::message<isRequest, Body, Fields>&& msg, asio::yield_context yield, error_code& ec) {
        _close = msg.need_eof();
        serializer<isRequest, Body, Fields> sr{msg};
        async_write(_stream, sr, yield[ec]);
        return msg.need_eof();
    }

    template<class T> struct always_false : std::false_type {};

    struct async_read_request_visitor {
        beast::tcp_stream& _stream;
        beast::flat_buffer& _buf;
        asio::yield_context _yield;
        error_code& _ec;
        async_read_request_visitor(beast::tcp_stream& stream, beast::flat_buffer& buf, asio::yield_context yield, error_code& ec):
            _stream{stream},
            _buf{buf},
            _yield{std::move(yield)},
            _ec{ec} {
        }
        template <typename BodyType>
        request<BodyType> operator()(request_parser<BodyType>& p) {
            async_read(_stream, _buf, p, _yield[_ec]);
            return p.release();
        }
    };

    void read(asio::yield_context yield) {
        auto self {this->shared_from_this()};
        beast::error_code ec;
        beast::flat_buffer buffer;
        context ctx;
        async_read_request_visitor read_req_visitor{_stream, buffer, yield, ec};
        for (;;) {
            request_parser<empty_body> req0;
            async_read_header(_stream, buffer, req0, yield[ec]);
            // get associated route
            try {
                auto target = req0.get().target();
                std::string path{target.data(), target.size()};
                auto route = _router->route_for(path);
                router_parser_variant parser_var;
                route.assign_parser(parser_var, req0);
                std::visit([this, yield, &ec, &buffer, &ctx](auto&p) mutable {
                    async_read_some(_stream, buffer, p, yield[ec]);
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
                    send(std::move(resp), yield, ec);
                }, route(path, ctx));
                if (_close || ec)
                    break;
            } catch(const typename router_type::not_found&) {
                response<string_body> res {
                    std::piecewise_construct,
                    std::make_tuple("not found"),
                    std::make_tuple(net::http::status::not_found, 10) //ctx.request.version())
                };
            }
        }
        _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
    beast::tcp_stream _stream;
    router_ptr _router;
};

template <typename...BodyTypes>
class server {
    using session_type = session<BodyTypes...>;
public:
    using context_type = typename session_type::context;
    using route_return_type = typename session_type::router_return_type;

    server(asio::io_context& ctx, const tcp::endpoint& endpoint):
        _acceptor{ctx, endpoint},
        _socket{ctx} {
        asio::spawn(ctx, std::bind(&server::_accept, this, std::placeholders::_1));
    }

    template <typename ViewHandlerT>
    void add_route(const std::string&path, ViewHandlerT handler) {
        _router->add_route(path, handler);
    }

private:
    void _accept(asio::yield_context yield) {
        error_code ec;
        for (;;) {
            _acceptor.async_accept(_socket, yield[ec]);
            if (!ec) {
                auto sess = std::make_shared<session_type>(std::move(_socket),
                                                           _router);
                asio::spawn(_acceptor.get_executor(),
                            [sess] (asio::yield_context yield) {
                    sess->read(yield);
                });
            }
        }
    }

    tcp::acceptor _acceptor;
    tcp::socket _socket;
    typename session_type::router_ptr _router = std::make_shared<typename session_type::router_type>();
};

} // namespace http

}  // namespace xdev::net
