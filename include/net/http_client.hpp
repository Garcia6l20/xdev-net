#pragma once

#include <net/net.hpp>
#include <net/url.hpp>

#include <boost/asio/spawn.hpp>

#include <filesystem>

namespace xdev::net::http {
struct client {

    client(asio::io_context& ioc):
        _ioc{ioc},
        _resolver{_ioc},
        _stream{_ioc} {
    }

    ~client() {
        if (_connected)
            _stream.socket().shutdown(tcp::socket::shutdown_both);
    }

    void connect(url&&url_, asio::yield_context yield, error_code& ec) {
        _url = std::forward<url>(url_);
        std::string resolver_service = std::to_string(_url.port());
        auto const results = _resolver.async_resolve(_url.hostname(), resolver_service, yield[ec]);
        if(ec)
            return;
        _stream.expires_after(std::chrono::seconds(30));
        _stream.async_connect(results, yield[ec]);
        if (ec)
            return;
        _connected = true;
    }

    template <typename ResponsetBodyType = beast::http::string_body>
    void get(response<ResponsetBodyType>& response, asio::yield_context yield, boost::system::error_code& ec) {
        request<empty_body> req {http::verb::get, {_url.path().data(), _url.path().size()}, _http_version};
        process_request(req, response, yield, ec);
    }

    template <typename ResponsetBodyType = beast::http::string_body>
    void post(std::string&& body, response<ResponsetBodyType>& response, asio::yield_context yield, boost::system::error_code& ec) {
        request<string_body> req {http::verb::post, {_url.path().data(), _url.path().size()}, _http_version};
        req.body() = std::forward<std::string>(body);
        req.set(field::content_length, req.body().size());
        process_request(req, response, yield, ec);
    }

    template <typename ResponsetBodyType = beast::http::string_body>
    void post(const std::filesystem::path& filepath, response<ResponsetBodyType>& response, asio::yield_context yield, boost::system::error_code& ec) {
        net::http::file_body::value_type file;
        file.open(filepath.c_str(), boost::beast::file_mode::read, ec);
        if (ec)
            return;
        request<file_body> req {http::verb::post, {_url.path().data(), _url.path().size()}, _http_version};
        req.body() = std::move(file);
        req.set(field::content_length, req.body().size());
        process_request(req, response, yield, ec);
    }

    template <typename RequestBodyType = beast::http::string_body, typename ResponsetBodyType = beast::http::string_body>
    void process_request(beast::http::request<RequestBodyType>& request, beast::http::response<ResponsetBodyType>& response, asio::yield_context yield, boost::system::error_code& ec) {
        static const auto timeout = std::chrono::seconds(30);
        auto start_t = std::chrono::system_clock::now();
        while (!_connected) {
            asio::basic_waitable_timer<std::chrono::system_clock> t(_ioc, std::chrono::milliseconds(10));
            t.async_wait(yield);
            if (start_t - std::chrono::system_clock::now() >= timeout) {
                ec = make_error_code(asio::error::timed_out);
                return;
            }
        }
        std::string target = {_url.path().data(), _url.path().size()};
        request.set(http::field::host, _url.hostname());
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::async_write(_stream, request, yield[ec]);
        if(ec)
            return;

        beast::flat_buffer b;
        http::async_read(_stream, b, response, yield[ec]);
    }

private:
    asio::io_context& _ioc;
    tcp::resolver _resolver;
    beast::tcp_stream _stream;
    bool _connected = false;
    url _url;
    static const int _http_version = 10;


public:

    template <typename BodyType = beast::http::string_body>
    static void async_get(url&&url_, beast::http::response<BodyType>& response, asio::io_context& ioc, boost::system::error_code& ec) {
        asio::spawn(ioc, [&ioc, url = std::forward<url>(url_), &response, &ec](asio::yield_context yield) mutable {
            client clt {ioc};
            clt.connect(std::move(url), yield, ec);
            clt.get(response, yield, ec);
        });
    }

    template <typename BodyType = beast::http::string_body>
    static void async_post(url&&url_, std::string&& body, beast::http::response<BodyType>& response, asio::io_context& ioc, boost::system::error_code& ec) {
        asio::spawn(ioc, [&ioc,
                    url = std::forward<url>(url_),
                    body = std::forward<std::string>(body),
                    &response,
                    &ec] (asio::yield_context yield) mutable {
            client clt {ioc};
            clt.connect(std::move(url), yield, ec);
            clt.post(std::forward<std::string>(body), response, yield, ec);
        });
    }

    template <typename BodyType = beast::http::string_body>
    static void async_post(url&&url_, const std::filesystem::path& filepath, beast::http::response<BodyType>& response, asio::io_context& ioc, boost::system::error_code& ec) {
        asio::spawn(ioc, [&ioc,
                    url = std::forward<url>(url_),
                    &filepath,
                    &response,
                    &ec] (asio::yield_context yield) mutable {
            client clt {ioc};
            clt.connect(std::move(url), yield, ec);
            clt.post(filepath, response, yield, ec);
        });
    }
};
}  // namespace xdev::net
