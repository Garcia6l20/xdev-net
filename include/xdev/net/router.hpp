#pragma once

#include <xdev/net/router_details.hpp>

#include <variant>

namespace xdev::net::http {

struct _dumb_route_context {
};

template <typename RouterContextT = _dumb_route_context,
          typename...BodyTypes>
class base_router
{
public:

    using body_traits = details::body_traits<BodyTypes...>;
    using return_type = typename body_traits::response_variant;
    using init_return_type = std::tuple<typename body_traits::body_variant, typename body_traits::body_value_variant>;
    using parser_type = typename body_traits::parser_variant;
    using context_type = RouterContextT;

    struct not_found: std::exception {
    };

    base_router() = default;

    using request_parser_assign_func = std::function<void(parser_type&, request_parser<empty_body>&)>;

    template <typename ViewHandler, typename UpstreamBodyType = string_body>
    void add_route(const std::string& path, ViewHandler view_handler) {
        request_parser_assign_func pc = [](parser_type& var, request_parser<empty_body>&req) {
            var.template emplace<request_parser<UpstreamBodyType>>(std::move(req));
        };
        _routes.push_back({path, view_handler, pc});
    }

    template <typename InitHandler, typename ViewHandler>
    void add_route(const std::string& path, InitHandler init_handler, ViewHandler view_handler) {
        _routes.push_back({path, init_handler, view_handler});
    }

    struct route {
        using handler_type = std::function<return_type(std::smatch, RouterContextT&)>;
        using init_handler_type = std::function<init_return_type(std::smatch, RouterContextT&)>;
        using data_type = std::shared_ptr<void>;
        struct not_matching: std::exception {
        };

        static void escape(std::string& input) {
            std::regex special_chars { R"([.])" };
            std::regex_replace(input, special_chars, R"(\$&)");
        }

        template <typename ViewHandler>
        route(const std::string& path, ViewHandler handler, request_parser_assign_func assign_parser = nullptr):
            _assign_parser{assign_parser} {
            using traits = details::view_handler_traits<return_type, RouterContextT, ViewHandler>;
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
        template <typename InitHandler, typename ViewHandler>
        route(const std::string& path, InitHandler init_handler, ViewHandler handler):
            route(path, handler) {
            using traits = details::view_handler_traits<init_return_type, RouterContextT, InitHandler>;
            auto data = std::make_shared<typename traits::data_type>();
            _init_handler = [this, data, init_handler](const std::smatch& match, RouterContextT& ctx) {
                traits::load_data(match, *data);
                return traits::invoke(init_handler, *data, ctx);
            };
        }

        bool operator()(const std::string& url) {
            return std::regex_match(url, _regex);
        }

        return_type operator()(const std::string& url, RouterContextT& ctx) {
            std::smatch match;
            if (std::regex_match(url, match, _regex)) {
                return _handler(match, ctx);
            }
            throw not_matching();
        }

        void init_route(const std::string& url, RouterContextT& ctx, parser_type& var, request_parser<empty_body>&req) {
            std::smatch match;
            if (std::regex_match(url, match, _regex)) {
                if (!_init_handler) {
                    _assign_parser(var, req);
                    return;
                } else {
                    using parser_variant = typename body_traits::parser_variant;
                    using body_value_variant = typename body_traits::body_value_variant;
                    auto init_tup = _init_handler(match, ctx);
                    std::visit([this,&var,&req, &init_tup](auto&body) {
                        std::visit([this,&var,&req, &body](auto&&body_value) {
                            using BodyT = std::decay_t<decltype(body)>;
                            using BodyValueT = std::decay_t<decltype(body_value)>;
                            if constexpr (std::is_same_v<typename BodyT::value_type, BodyValueT>) {
                                var.template emplace<request_parser<BodyT>>(std::move(req));
                                std::get<request_parser<BodyT>>(var).get().body() = std::move(body_value);
                            } else {
                                throw std::runtime_error("Body type mismatch");
                            }
                        }, std::get<1>(init_tup));
                    }, std::get<0>(init_tup));
                }
            } else {
                throw not_matching();
            }
        }

    private:

        request_parser_assign_func _assign_parser;
        std::regex _regex;
        handler_type _handler;
        init_handler_type _init_handler;
    };

    const route& route_for(const std::string& url) {
        for (auto& route: _routes) {
            if (route(url))
                return route;
        }
        throw not_found();
    }

    return_type operator()(const std::string& url) {
        RouterContextT ctx{};
        return operator()(url, ctx);
    }
    return_type operator()(const std::string& url, RouterContextT& ctx) {
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

using simple_router = base_router<_dumb_route_context, string_body, file_body, dynamic_body>;

} // namespace xdev
