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

    struct context {
        beast::http::request<string_body> request;
    };

    using router_return_type = std::variant<response<string_body>,
                                            response<file_body>,
                                            response<dynamic_body>,
                                            BodyTypes...>;
    using router_type = base_router<router_return_type, context>;
    using router_ptr = std::shared_ptr<router_type>;

    bool _close;

    session(tcp::socket socket, router_ptr router):
        _socket{std::move(socket)}, _router{router} {
    }

    template<bool isRequest, class Body, class Fields>
    bool send(beast::http::message<isRequest, Body, Fields>&& msg, asio::yield_context yield, error_code& ec) {
        _close = msg.need_eof();
        serializer<isRequest, Body, Fields> sr{msg};
        async_write(_socket, sr, yield[ec]);
        return msg.need_eof();
    }

    void read(asio::yield_context yield) {
        auto self {this->shared_from_this()};
        beast::error_code ec;
        beast::flat_buffer buffer;
        context ctx;
        for (;;) {
            async_read(_socket, buffer, ctx.request, yield[ec]);
            if (ec == http::error::end_of_stream)
                break;
            if  (ec)
                throw std::runtime_error(ec.message());
            auto target = ctx.request.target();
            std::visit([this, yield, &ec, &ctx](auto&&resp) {
                resp.version(ctx.request.version());
                resp.keep_alive(ctx.request.keep_alive());
                send(std::move(resp), yield, ec);
            }, (*_router)({target.data(), target.size()}, ctx));
            if (_close || ec)
                break;
        }
        _socket.shutdown(tcp::socket::shutdown_send, ec);
    }
    tcp::socket _socket;
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
    void operator()(const std::string&path, ViewHandlerT handler) {
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
