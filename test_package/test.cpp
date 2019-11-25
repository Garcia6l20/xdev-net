#include <xdev/net.hpp>

#include <iostream>

using namespace xdev;

int main() try {
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

    std::atomic_bool ready = false;;

    auto fut = std::async([&srvctx, &ready] {
        srvctx.poll_one();
        ready = true;
        srvctx.run();
    });

    while (!ready)
        std::this_thread::yield();

    boost::asio::io_context ctx;
    net::error_code ec;
    net::http::response<net::http::string_body> reply;
    net::http::client::async_get({"http://localhost:4242/add/20.1/21.9"}, reply, ctx, ec);

    ctx.run();

    srvctx.stop();
    
    return 0;
} catch(const std::exception& err) {
    std::cerr << "exception raised: " << err.what() << std::endl;
    return -1;
}
