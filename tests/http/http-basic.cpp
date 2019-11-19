#include <net-test.hpp>

#include <net/http_server.hpp>
#include <net/http_client.hpp>
#include <net/http_reply.hpp>

#include <iostream>

using namespace std::literals::string_literals;
using namespace std::literals::chrono_literals;
using namespace xdev;

struct HttpBasicTest: testing::NetTest {
    net::address addr{"localhost", 4242};
};

TEST_F(HttpBasicTest, Nominal) {
    net::http_server srv;
    srv("/show_request", [](net::http_server::context& context) {
        auto req = context.request;
        std::cout << "path: " << req.path << std::endl;
        if (!req.headers.empty()) {
            std::cout << "headers: " << std::endl;
            for (const auto& [field, value]: req.headers)
                std::cout << " - " << field << ": " << value << std::endl;
        }
        std::cout << "method: " << req.method_str() << std::endl;
        context.send_reply(net::http_basic_body_reply{"ok"});
    });
    auto listen_guard = srv.start_listening(addr);
    ASSERT_EQ("ok", net::http_client::get({"http://localhost:4242/show_request"}).body.string_view());
}

TEST_F(HttpBasicTest, NotFount) {
    net::http_server srv;
    auto listen_guard = srv.start_listening(addr);
    ASSERT_EQ(net::http_status::HTTP_STATUS_NOT_FOUND, net::http_client::get({ "http://localhost:4242/test" }).status);
}

TEST_F(HttpBasicTest, InternalError) {
    net::http_server srv;
    srv("/runtime_error", [](net::http_server::context&) {
        throw std::runtime_error("test");
    });
    srv("/undefined_error", [](net::http_server::context&) {
        throw 0;
    });
    auto listen_guard = srv.start_listening(addr);
    auto reply = net::http_client::get({ "http://localhost:4242/runtime_error" });
    ASSERT_EQ(net::http_status::HTTP_STATUS_INTERNAL_SERVER_ERROR, reply.status);
    ASSERT_EQ("test", reply.body.string_view());
    reply = net::http_client::get({ "http://localhost:4242/undefined_error" });
    ASSERT_EQ(net::http_status::HTTP_STATUS_INTERNAL_SERVER_ERROR, reply.status);
    ASSERT_EQ("unknown error", reply.body.string_view());
}

TEST_F(HttpBasicTest, Add) {
    net::http_server srv;
    srv("/add/<a>/<b>", [](double a, double b, net::http_server::context& context) {
        context.send_reply(net::http_basic_body_reply{std::to_string(a + b)});
    });
    auto listen_guard = srv.start_listening(addr);
    ASSERT_NEAR(42.0, std::stod(std::string(net::http_client::get({"http://localhost:4242/add/20.2/21.8"}).body.string_view())), 0.001);
}

#include <fstream>

struct ifstream_reply: net::http_reply {
    ifstream_reply(const std::string& filename):
        _ifs(filename, std::ifstream::in) {
    }

    // StreamBodyProvider
    std::istream& body() {
        return _ifs;
    }

    // HttpHeadProvider
    std::string http_head() {
        std::streampos fsize = 0;
        fsize = _ifs.tellg();
        _ifs.seekg(0, std::ios::end);
        fsize = _ifs.tellg() - fsize;
        _ifs.seekg(0, std::ios::beg);
        headers["Content-Length"] = std::to_string(fsize);
        return http_reply::http_head();
    }
private:
    std::ifstream _ifs;
};

TEST_F(HttpBasicTest, Stream) {
    net::http_server srv;
    srv("/get_this_test", [](net::http_server::context& context) {
        context.send_reply(ifstream_reply{__FILE__});
    });
    auto listen_guard = srv.start_listening(addr);
    auto reply = net::http_client::get({"http://localhost:4242/get_this_test"});

    std::streampos fsize = 0;
    std::ifstream file(__FILE__, std::ios::ate);
    fsize = file.tellg();
    file.close();

    std::cout << reply.body.string_view() << std::endl;
    ASSERT_EQ(net::http_status::HTTP_STATUS_OK, reply.status);
    ASSERT_EQ(fsize, std::stoll(reply.headers["Content-Length"]));
}
