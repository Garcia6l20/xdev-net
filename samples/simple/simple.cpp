#include <iostream>

#include <xdev/net.hpp>

#include <boost/beast/http/chunk_encode.hpp>

using namespace xdev;

int main() {
    boost::asio::io_context srvctx;

    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.on("/add/<a>/<b>").complete([](double a, double b, server_type::context_type& context) {
        auto response = context.make_response();
        response.body() = std::to_string(a + b);
        response.set(net::http::field::content_type, "text/plain");
        response.content_length(response.body().size());
        return response;
    });

    srv.on("/chunked").complete([](server_type::context_type& context) {
        auto response = context.make_response();
        auto& request = context.req();
        std::cout << "chunked: " << request.chunked() << std::endl;
        std::cout << "body: " << request.body() << std::endl;
        response.body() = "ok";
        response.set(net::http::field::content_type, "text/plain");
        response.content_length(response.body().size());
        return response;
    });

    srvctx.run();
}
