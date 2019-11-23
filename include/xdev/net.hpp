#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#if !defined(__cpp_concepts)
# ifdef _WIN32
# pragma message ("Constrains are not enabled in pre cxx20")
# else
# warning "Constrains are not enabled in pre cxx20"
# endif
#endif

namespace xdev::net {
    using error_code = boost::system::error_code;
    namespace beast = boost::beast;
    namespace asio = boost::asio;
    namespace ip = boost::asio::ip;
    namespace ssl = boost::asio::ssl;
    using tcp = ip::tcp;

    namespace http {
        using namespace beast::http;
    }
}  // namespace xdev::net

#include <xdev/net/http_client.hpp>
#include <xdev/net/http_server.hpp>
