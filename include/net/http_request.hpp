#pragma once

#include <net/http_parser.hpp>

#include <map>
#include <sstream>

namespace xdev::net {

using http_headers = std::map<std::string, std::string>;

using http_request_parser = http_parser<http_parser_type::HTTP_REQUEST>;

struct http_request {
    http_method method;
    http_headers headers;
    std::string path;
    buffer body;

    std::string method_str() const {
        return http_method_str(method);
    }

    static http_request build(http_request_parser& parser) {
        http_request req{
            .method = static_cast<http_method>(parser.method),
            .headers = std::move(parser.headers),
            .path = std::move(parser.url),
            .body = std::move(parser.body),
        };
        return std::move(req);
    }

    std::string http_head() const {
        std::ostringstream ss;
        ss << method_str() << " " << path << " " << "HTTP/1.1\r\n";
        for (auto&& [field, value] : headers)
            ss << field << ": " << value << "\r\n";
        ss << "\r\n";
        return ss.str();
    }
};


}  // namespace xdev::net
