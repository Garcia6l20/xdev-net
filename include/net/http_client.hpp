#pragma once

#include <net/net.hpp>
#include <net/url.hpp>

#include <boost/asio/spawn.hpp>

namespace xdev::net::http {

    struct client {
        template <typename BodyType = beast::http::string_body>
        static asio::io_context& async_get(url&&url_, beast::http::response<BodyType>& response, asio::io_context& ioc, boost::system::error_code& ec) {
            asio::spawn([&ec, &ioc, url = std::forward<url>(url_), &response](asio::yield_context yield) {
                tcp::resolver resolver(ioc);
                beast::tcp_stream stream(ioc);
                std::string resolver_service = std::to_string(url.port());
                auto const results = resolver.async_resolve(url.hostname(), resolver_service, yield[ec]);
                if(ec)
                    return;
                stream.expires_after(std::chrono::seconds(30));
                stream.async_connect(results, yield[ec]);
                if (ec)
                    return;
                namespace http = beast::http;
                std::string target = {url.path().data(), url.path().size()};
                http::request<http::string_body> req{http::verb::get, target, 10};
                req.set(http::field::host, url.hostname());
                req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                http::async_write(stream, req, yield[ec]);
                if(ec)
                    return;

                beast::flat_buffer b;
                http::response<BodyType> res;
                http::async_read(stream, b, res, yield[ec]);
                if(ec)
                    return;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                response = std::move(res);
            });
            return ioc;
        }
    };
}  // namespace xdev::net
