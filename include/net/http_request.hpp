#pragma once

#include <net/http_parser.hpp>

#include <map>

namespace xdev::net {

using http_headers = std::map<std::string, std::string>;

using http_request_parser = http_parser<http_parser_type::HTTP_REQUEST>;

struct http_request {
    http_method method;
    http_headers headers;
    std::string url;
    std::string body;
    std::string status;

    std::string method_str() const {
        return http_method_str(method);
    }

    static http_request build(http_request_parser& parser) {
        http_request req{
            .method = static_cast<http_method>(parser.method),
            .headers = std::move(parser.headers),
            .url = std::move(parser.url),
            .body = std::move(parser.body),
            .status = std::move(parser.status),
        };
        return std::move(req);
    }
};


}  // namespace xdev::net
