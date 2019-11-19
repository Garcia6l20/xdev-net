#pragma once

#include <string_view>
#include <string>

namespace xdev::net {

namespace node {
#include <http_parser.h>
}

using http_status = node::http_status;
const auto http_status_str = node::http_status_str;

using http_method = node::http_method;
const auto http_method_str = node::http_method_str;

using http_parser_type = node::http_parser_type;

template <http_parser_type _type = http_parser_type::HTTP_BOTH>
struct http_parser: node::http_parser {

    http_parser() {
        node::http_parser_init(this, http_parser_type(_type));
    }

    std::string error_description() const {
        return {http_errno_description(node::http_errno(http_errno))};
    }

    bool operator()(std::string_view data) {
        return node::http_parser_execute(this, &_parser_settings, data.data(), data.length()) == data.length();
    }

    operator bool() const {
        return complete;
    }

    std::string url;
    std::string status;
    buffer body;
    std::map<std::string, std::string> headers;
    bool complete = false;
    
private:
    const char* _field = nullptr;
    size_t _field_length = 0;

    static http_parser& instance(node::http_parser* p) {
        return *static_cast<http_parser*>(p);
    }

    static int _on_msg_begin(node::http_parser* p) {
        instance(p).complete = false;
        return 0;
    }
    static int _on_url(node::http_parser* p, const char *at, size_t length) {
        instance(p).url = {at, length};
        return 0;
    }
    static int _on_status(node::http_parser* p, const char *at, size_t length) {
        instance(p).status = {at, length};
        return 0;
    }
    static int _on_header_field(node::http_parser* p, const char *at, size_t length) {
        instance(p)._field = at;
        instance(p)._field_length = length;
        return 0;
    }
    static int _on_header_value(node::http_parser* p, const char *at, size_t length) {
        auto me = instance(p);
        me.headers[{me._field, me._field_length}] = {at, length};
        me._field = nullptr;
        me._field_length = 0;
        return 0;
    }
    static int _on_headers_complete(node::http_parser* p) {
        return 0;
    }
    static int _on_body(node::http_parser* p, const char *at, size_t length) {
        if (instance(p).body.size() < length)
            instance(p).body.resize(length);
        std::copy(at, at + length, instance(p).body.begin());
        return 0;
    }
    static int _on_msg_complete(node::http_parser* p) {
        instance(p).complete = true;
        return 0;
    }
    static int _on_chunk_header(node::http_parser* p) {
        throw 0;
    }
    static int _on_chunk_complete(node::http_parser* p) {
        throw 0;
    }

    node::http_parser_settings _parser_settings = {
        &_on_msg_begin,
        &_on_url,
        &_on_status,
        &_on_header_field,
        &_on_header_value,
        &_on_headers_complete,
        &_on_body,
        &_on_msg_complete,
        &_on_chunk_header,
        &_on_chunk_complete
    };
};

}  // namespace xdev::net
