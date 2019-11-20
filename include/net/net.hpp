#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>

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
    using tcp = ip::tcp;

    namespace http {
        using namespace beast::http;
    }
}  // namespace xdev::net
