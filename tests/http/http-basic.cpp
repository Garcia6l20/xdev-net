#include <net-test.hpp>

#include <net/http_server.hpp>
#include <net/http_client.hpp>

#include <iostream>
#include <fstream>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

struct HttpBasicTest: testing::NetTest {
};

TEST_F(HttpBasicTest, Nominal) {
    boost::asio::io_context srvctx;
    using server_type = net::http::server<>;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv("/test", [](server_type::context_type& context) -> server_type::route_return_type {
        net::http::response<net::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple("ok"),
            std::make_tuple(net::http::status::ok, context.request.version())
        };
        res.set(net::http::field::content_type, "text/plain");
        res.content_length(res.body().size());
        return std::move(res);
    });

    auto fut = std::async([&srvctx] {
        srvctx.run();
    });

    std::this_thread::sleep_for(500ms);

    boost::asio::io_context ctx;
    net::error_code ec;
    net::http::response<net::http::string_body> reply;
    net::http::client::async_get({"http://localhost:4242/test"}, reply, ctx, ec);

    ctx.run();

    ASSERT_EQ("ok", reply.body());

    srvctx.stop();
}

TEST(HttpBoostTest, CoroFileRead) {
    boost::asio::io_context srvctx;
    using server_type = net::http::server<>;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv("/get_this_test", [](server_type::context_type& context) -> server_type::route_return_type {
        net::error_code ec;
        net::http::response<net::http::file_body> resp;
        net::http::file_body::value_type file;
        file.open(__FILE__, boost::beast::file_mode::read, ec);
        if (ec)
            throw std::runtime_error(ec.message());
        resp.body() = std::move(file);
        return std::move(resp);
    });

    auto fut = std::async([&srvctx] {
        srvctx.run();
    });

    std::this_thread::sleep_for(500ms);

    boost::beast::http::response<boost::beast::http::string_body> reply;
    net::asio::io_context ctx;
    net::error_code ec;
    net::http::client::async_get({"http://localhost:4242/get_this_test"}, reply, ctx, ec);
    ctx.run();

    std::streampos fsize = 0;
    std::ifstream file(__FILE__, std::ios::ate);
    fsize = file.tellg();
    file.close();

    std::cout << reply.body() << std::endl;

    srvctx.stop();
}
