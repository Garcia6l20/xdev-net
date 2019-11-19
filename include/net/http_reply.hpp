#pragma once

#include <net/http_parser.hpp>
#include <net/http_request.hpp>
#include <net/socket.hpp>

#include <sstream>

namespace xdev::net {

using http_reply_parser = http_parser<http_parser_type::HTTP_RESPONSE>;

struct http_reply {

    http_reply() = default;
    http_reply(std::string_view body) :
        _body{ body.begin(), body.end() } {
    }
    http_reply(http_status status, std::string_view body) :
        http_reply(body) {
        _status = status;
    }

    http_status _status = http_status::HTTP_STATUS_OK;
    http_headers _headers = { {{"Content-Type"}, {"application/x-empty"}} };
    buffer _body;

    std::string http_head() {
        _headers["Content-Length"] = std::to_string(_body.size());
        std::ostringstream ss;
        ss << "HTTP/1.1 " << _status << " " << http_status_str(_status) << "\r\n";
        for (auto&& [field, value] : _headers)
            ss << field << ": " << value << "\r\n";
        ss << "\r\n";
        return ss.str();
    }

    buffer body() {
        return std::move(_body);
    }

    static http_reply load(http_reply_parser& p) {
        http_reply reply;
        reply._status = static_cast<http_status>(p.status_code);
        reply._headers = std::move(p.headers);
        reply._body = std::move(p.body);
        return reply;
    }
};
}  // namespace xdev::net
