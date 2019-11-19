#pragma once

#include <net/url.hpp>
#include <net/http_reply.hpp>
#include <net/http_request.hpp>
#include <net/tcp.hpp>

namespace xdev::net {
    struct http_client: private net::tcp_client {
        static http_reply get(url&& url) {
            http_client client{};
            client.connect({ std::string{url.hostname()}, url.port() });
            http_request req;
            req.headers["User-Agent"] = "xdev/0.1";
            req.method = http_method::HTTP_GET;
            req.path = url.path().empty() ? "/" : url.path();
            client.send(req.http_head());
            http_reply_parser parser;
            while (!parser) {
                auto fut = client.receive();
                auto [data, _] = fut.get();
                parser({data.data(), data.size()});
            }
            return http_reply::load(parser);
        }
    };
}  // namespace xdev::net
