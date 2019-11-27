#include <iostream>

#include <xdev/net.hpp>

#include <filesystem>
#include <fstream>

using namespace xdev;
using namespace std::string_literals;

int main() {
    boost::asio::io_context srvctx;

    using server_type = net::http::plain_server;
    server_type srv{srvctx, {net::ip::address_v4::loopback(), 4242}};
    srv.on("/add/<a>/<b>")
    .complete([](double a, double b) {
        return std::to_string(a + b);
    });

    srv.on("/chunked-upload/<path>")
    .init([](const std::filesystem::path& path, server_type::context_type& context) {
        std::ofstream stream(path.string().c_str(), std::ofstream::out);
        context.make_data<std::ofstream>(path.string().c_str(), std::ofstream::out);
    })
    .chunk([](std::string_view data, server_type::context_type& context) {
        std::cout << "got chunk: " << data << std::endl;
        auto& stream = context.data<std::ofstream>();
        stream << data;
    })
    .complete([](const std::filesystem::path& path, server_type::context_type& context) {
        context.data<std::ofstream>().close();
        auto& request = context.req();
        if (request.begin() != request.end()) {
            std::cout << "headers: " << std::endl;
            for (const auto& header: request) {
                std::cout << " - " << header.name() << ": " << header.value() << std::endl;
            }
        }
        std::cout << "chunked: " << request.chunked() << std::endl;
        std::cout << "method: " << request.method_string() << std::endl;
        std::cout << "need_eof: " << request.need_eof() << std::endl;
        return "ok"s;
    });

    srvctx.run();
}
