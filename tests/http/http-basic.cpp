#include <net-test.hpp>

#include <net/http_server.hpp>
#include <net/http_client.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

TEST(HttpBasicTest, Nominal) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.add_route("/test", [](server_type::context_type& context) -> server_type::route_return_type {
        auto request = context.req();
        net::http::response<net::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple("ok"),
            std::make_tuple(net::http::status::ok, request.version())
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

TEST(HttpBasicTest, FileRead) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.add_route("/get_this_test", [](server_type::context_type& context) -> server_type::route_return_type {
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

TEST(HttpBasicTest, Add) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.add_route("/add/<a>/<b>", [](double a, double b, server_type::context_type& context) -> server_type::route_return_type {
        auto request = context.req();
        net::http::response<net::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(std::to_string(a + b)),
            std::make_tuple(net::http::status::ok, request.version())
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
    net::http::client::async_get({"http://localhost:4242/add/20.1/21.9"}, reply, ctx, ec);

    ctx.run();

    ASSERT_NEAR(42.0, std::stod(reply.body()), 0.001);

    srvctx.stop();
}

TEST(HttpBasicTest, FileUpload) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.add_route("/upload/<path>", [](const std::filesystem::path& path, server_type::context_type& context) -> server_type::route_init_return_type {
        using namespace std::filesystem;
        net::error_code ec;
        if (!exists(path.parent_path())) {
            create_directories(path.parent_path());
        }
        net::http::file_body::value_type file;
        file.open(path.c_str(), boost::beast::file_mode::write, ec);
        if (ec)
            throw std::runtime_error(ec.message());
        return {net::http::file_body{}, std::move(file)};
    }, [](const std::filesystem::path& path, server_type::context_type& context) -> server_type::route_return_type {
        auto& request = context.req<net::http::file_body>();
        net::http::response<net::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple(path.string() + " uploaded"),
            std::make_tuple(net::http::status::ok, request.version())
        };
        res.set(net::http::field::content_type, "text/plain");
        res.content_length(res.body().size());
        return std::move(res);
    });

    auto fut = std::async([&srvctx] {
        try {
            srvctx.run();
        } catch(const std::exception& err) {
            std::cerr << err.what() << std::endl;
        }
    });

    std::this_thread::sleep_for(500ms);

    boost::asio::io_context ctx;
    net::error_code ec;
    net::http::response<net::http::string_body> reply;
    net::http::client::async_post({"http://localhost:4242/upload/test/test.txt"}, std::string("hello world"), reply, ctx, ec);

    ctx.run();

    ASSERT_EQ("test/test.txt uploaded", reply.body());

    std::ifstream t("test/test.txt");
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    ASSERT_EQ("hello world", str);

    srvctx.stop();
}

template <size_t sz>
struct ro_array_body {
    using value_type = std::array<char, sz>;

    static
    std::uint64_t
    size(value_type const& body)
    {
        return body.size();
    }

    class reader {

        value_type& _body;
        size_t _offset = 0;

    public:
        template<bool isRequest, class Fields>
        explicit
        reader(net::http::header<isRequest, Fields>&, value_type& b)
            : _body(b)
        {
        }

        void
        init(boost::optional<
            std::uint64_t> const& length, net::error_code& ec)
        {
            if(length)
            {
                if(*length > sz)
                {
                    ec = net::http::error::buffer_overflow;
                    return;
                }
            }
            ec = {};
        }

        template<class ConstBufferSequence>
        std::size_t
        put(ConstBufferSequence const& buffers,
            net::error_code& ec)
        {
            auto const extra = net::beast::buffer_bytes(buffers);
            if (extra > sz - _offset)
            {
                ec = net::http::error::buffer_overflow;
                return 0;
            }

            ec = {};

            char* dest = &_body[_offset];
            for(auto b : net::beast::buffers_range_ref(buffers))
            {
                std::memcpy(dest, b.data(), b.size());
                dest += b.size();
                _offset += b.size();
            }

            return extra;
        }

        void
        finish(net::error_code& ec)
        {
            ec = {};
        }
    };

    class writer
    {
    public:
        using const_buffers_type =
            net::beast::net::const_buffer;

        template<bool isRequest, class Fields>
        explicit
        writer(net::http::header<isRequest, Fields> const&, value_type const& b)
        {
            throw std::runtime_error("read only");
        }

        void
        init(net::error_code& ec)
        {
            throw std::runtime_error("read only");
        }

        boost::optional<std::pair<const_buffers_type, bool>>
        get(net::error_code& ec)
        {
            throw std::runtime_error("read only");
        }
    };
};

TEST(HttpBasicTest, CustomBody) {
    boost::asio::io_context srvctx;
    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.add_route("/test", [](server_type::context_type& context) -> server_type::route_return_type {
        auto request = context.req();
        net::http::response<net::http::string_body> res{
            std::piecewise_construct,
            std::make_tuple("ok"),
            std::make_tuple(net::http::status::ok, request.version())
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
    net::http::response<ro_array_body<2>> reply;
    net::http::client::async_get({"http://localhost:4242/test"}, reply, ctx, ec);

    ctx.run();

    auto array = std::move(reply.body());

    ASSERT_EQ('o', array[0]);
    ASSERT_EQ('k', array[1]);

    srvctx.stop();
}

