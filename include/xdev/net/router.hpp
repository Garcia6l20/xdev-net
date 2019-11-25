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

    struct route {
        using handler_type = std::function<return_type(std::smatch, RouterContextT&)>;
        using init_handler_type = std::function<init_return_type(std::smatch, RouterContextT&)>;
        using data_type = std::shared_ptr<void>;
        struct not_matching: std::exception {
        };

        static void escape(std::string& input) {
            std::regex special_chars { R"([-[\]{}()*+?.,\^$|#\s])" };
            std::regex_replace(input, special_chars, R"(\$&)");
        }

        route(const std::string& path):
            _path {path},
            _path_escaped {path} {
            escape(_path_escaped);
        }

        route(route&&) = default;
        route& operator=(route&&) = default;

        route(const route&) = delete ;
        route& operator=(const route&) = delete;

        template <typename HandlerT>
        route& init(HandlerT handler) {
            if (_handler) {
                throw std::logic_error("init must be called() before complete()");
            }
            using traits = details::view_handler_traits<init_return_type, RouterContextT, HandlerT>;
            auto data = std::make_shared<typename traits::data_type>();
            _init_handler = [this, data, handler](const std::smatch& match, RouterContextT& ctx) {
                traits::load_data(match, *data);
                return traits::invoke(handler, *data, ctx);
            };
            return *this;
        }

        template <typename HandlerT, typename UpstreamBodyType = string_body>
        route& complete(HandlerT handler) {
            using traits = details::view_handler_traits<return_type, RouterContextT, HandlerT>;
            if (!_init_handler) {
                // create default parser assigner
                _assign_parser = [](parser_type& var, request_parser<empty_body>&req) {
                    var.template emplace<request_parser<UpstreamBodyType>>(std::move(req));
                };
            }
            _regex = traits::make_regex(_path_escaped);
            auto data = std::make_shared<typename traits::data_type>();
            _handler = [this, data, handler](const std::smatch& match, RouterContextT& ctx) {
                traits::load_data(match, *data);
                return traits::invoke(handler, *data, ctx);
            };
            return *this;
        }

        std::tuple<bool, std::smatch> operator()(const std::string& url) {
            std::smatch match;
            return {std::regex_match(url, match, _regex), std::move(match)};
        }

        return_type operator()(const std::smatch& match, RouterContextT& ctx) {
            return _handler(match, ctx);
        }

        void init_route(const std::smatch& match, RouterContextT& ctx, parser_type& var, request_parser<empty_body>&req) {
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
        }

    private:
        std::string _path, _path_escaped;
        request_parser_assign_func _assign_parser;
        std::regex _regex;
        handler_type _handler;
        init_handler_type _init_handler;
    };

    route& add_route(const std::string& path) {
        return _routes.emplace_back(path);
    }

    std::tuple<route&, std::smatch> route_for(const std::string& url) {
        for (auto& route: _routes) {
            auto res = route(url);
            if (std::get<0>(res))
                return {route, std::move(std::get<1>(res))};
        }
        throw not_found();
    }

private:

    std::vector<route> _routes;
};

using simple_router = base_router<_dumb_route_context, string_body, file_body, dynamic_body>;

} // namespace xdev
