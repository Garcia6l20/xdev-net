#include <net-test.hpp>

#include <xdev/net/http_server.hpp>
#include <xdev/net/http_client.hpp>

#include <iostream>
#include <queue>

using namespace xdev;

TEST(HttpChunkTest, Nominal) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    std::string data;
    srv.on("/test")
    .init([&data] {
        data.clear();
    })
    .chunk([&data] (std::string_view chunk) {
        data.append(chunk);
    })
    .complete([] {
        return std::string("ok");
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
    std::queue<std::string> chunks;
    chunks.push("hello");
    chunks.push(" ");
    chunks.push("world");
    chunks.push(" !!");
    net::http::client::async_post({"http://localhost:4242/test"}, [chunks=std::move(chunks)]() mutable -> std::optional<std::string> {
        if (chunks.empty())
            return {};
        auto&& chunk = std::move(chunks.front());
        chunks.pop();
        return std::move(chunk);
    }, reply, ctx, ec);

    ctx.run();

    ASSERT_EQ("hello world !!", data);
    ASSERT_EQ("ok", reply.body());

    srvctx.stop();
}
