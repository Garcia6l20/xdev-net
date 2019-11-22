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
    private:
        typename body_traits::request_variant _request_var;
        friend class session<BodyTypes...>;
    };

    using router_type = base_router<context, string_body, file_body, dynamic_body, BodyTypes...>;
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

    void read(asio::yield_context yield) {
        auto self {this->shared_from_this()};
        beast::error_code ec;
        beast::flat_buffer buffer;
        context ctx;
        for (;;) {
            request_parser<empty_body> req0;
            async_read_header(_stream, buffer, req0, yield[ec]);
            // get associated route
            try {
                auto target = req0.get().target();
                std::string path{target.data(), target.size()};
                auto route = _router->route_for(path);
                typename body_traits::parser_variant parser_var;
                route.init_route(path, ctx, parser_var, req0);
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
    using route_return_type = typename session_type::router_type::return_type;
    using route_init_return_type = typename session_type::router_type::init_return_type;

    server(asio::io_context& ctx, const tcp::endpoint& endpoint):
        _acceptor{ctx, endpoint},
        _socket{ctx} {
        asio::spawn(ctx, std::bind(&server::_accept, this, std::placeholders::_1));
    }

    template <typename ViewHandlerT>
    void add_route(const std::string&path, ViewHandlerT handler) {
        _router->add_route(path, handler);
    }

    template <typename BodyInitHandlerT, typename ViewHandlerT>
    void add_route(const std::string&path, BodyInitHandlerT init_handler, ViewHandlerT handler) {
        _router->add_route(path, init_handler, handler);
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
