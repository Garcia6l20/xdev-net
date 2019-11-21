#pragma once

#include <net/net.hpp>
#include <net/router_details.hpp>

#include <variant>

namespace xdev::net::http {

struct _dumb_route_context {
};

template <typename RouteRetT = void,
          typename RouterContextT = _dumb_route_context,
          typename ParserVariantT = std::variant<request_parser<string_body>,
                                                 request_parser<file_body>,
                                                 request_parser<dynamic_body>>
          >
class base_router
{
public:
    using return_type = RouteRetT;
    using context_type = RouterContextT;

    struct not_found: std::exception {
    };

    base_router() = default;

    using request_parser_assign_func = std::function<void(ParserVariantT&, request_parser<empty_body>&)>;

    template <typename ViewHandler, typename UpstreamBodyType = string_body>
    void add_route(const std::string& path, ViewHandler view_handler) {
        request_parser_assign_func pc = [](ParserVariantT& var, request_parser<empty_body>&req) {
            var.template emplace<request_parser<UpstreamBodyType>>(std::move(req));
        };
        _routes.push_back({path, view_handler, pc});
    }

    struct route {
        using handler_type = std::function<RouteRetT(std::smatch, RouterContextT&)>;
        using data_type = std::shared_ptr<void>;
        struct not_matching: std::exception {
        };

        static void escape(std::string& input) {
            std::regex special_chars { R"([.])" };
            std::regex_replace(input, special_chars, R"(\$&)");
        }

        template <typename ViewHandler>
        route(const std::string& path, ViewHandler handler, request_parser_assign_func assign_parser):
            assign_parser{assign_parser} {
            using traits = details::view_handler_traits<RouteRetT, RouterContextT, ViewHandler>;
            std::string path_regex = path;
            escape(path_regex);
            std::smatch match;
            _regex = traits::make_regex(path);
            auto data = std::make_shared<typename traits::data_type>();
            _handler = [this, data, handler](const std::smatch& match, RouterContextT& ctx) {
                traits::load_data(match, *data);
                return traits::invoke(handler, *data, ctx);
            };
        }

        bool operator()(const std::string& url) {
            return std::regex_match(url, _regex);
        }

        RouteRetT operator()(const std::string& url, RouterContextT& ctx) {
            std::smatch match;
            if (std::regex_match(url, match, _regex)) {
                return _handler(match, ctx);
            }
            throw not_matching();
        }

        request_parser_assign_func assign_parser;

    private:
        std::regex _regex;
        handler_type _handler;
    };

    const route& route_for(const std::string& url) {
        for (auto& route: _routes) {
            if (route(url))
                return route;
        }
        throw not_found();
    }

    RouteRetT operator()(const std::string& url) {
        for (auto& route: _routes) {
            try {
                _dumb_route_context ctx;
                return route(url, ctx);
            } catch (const typename route::not_matching&) {}
        }
        throw not_found();
    }
    RouteRetT operator()(const std::string& url, RouterContextT& ctx) {
        for (auto& route: _routes) {
            try {
                return route(url, ctx);
            } catch (const typename route::not_matching&) {}
        }
        throw not_found();
    }

private:

    std::vector<route> _routes;
};

using simple_router = base_router<>;

} // namespace xdev
