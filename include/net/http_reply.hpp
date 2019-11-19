#pragma once

#include <net/http_parser.hpp>
#include <net/http_request.hpp>
#include <net/socket.hpp>

#include <sstream>

namespace xdev::net {

using http_reply_parser = http_parser<http_parser_type::HTTP_RESPONSE>;

struct http_reply {

    http_reply() = default;

    http_status status = http_status::HTTP_STATUS_OK;
    http_headers headers = { {{"Content-Type"}, {"application/x-empty"}} };

    static http_reply load(http_reply_parser& p) {
        http_reply reply;
        reply.status = static_cast<http_status>(p.status_code);
        reply.headers = std::move(p.headers);
        return reply;
    }

    std::string http_head() {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << status << " " << http_status_str(status) << "\r\n";
        for (auto&& [field, value] : headers)
            ss << field << ": " << value << "\r\n";
        ss << "\r\n";
        return ss.str();
    }
};

struct http_basic_body_reply : http_reply {

    buffer body;

    http_basic_body_reply() = default;

    http_basic_body_reply(std::string_view body) :
        body{ body.begin(), body.end() } {
    }
    http_basic_body_reply(http_status p_status, std::string_view body) :
        http_basic_body_reply(body) {
        status = p_status;
    }

    http_basic_body_reply(http_reply&&base): http_reply(std::move(base)) {}

    std::string http_head() {
        headers["Content-Length"] = std::to_string(body.size());
        return http_reply::http_head();
    }

    static http_basic_body_reply load(http_reply_parser& p) {
        http_basic_body_reply reply = http_reply::load(p);
        reply.body = std::move(p.body);
        return reply;
    }

};

}  // namespace xdev::net
