#pragma once

#include <xdev/net.hpp>

#include <xdev/variant_tools.hpp>
#include <xdev/net/config.hpp>
#include <xdev/net/router_details.hpp>

#include <boost/type_index.hpp>

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
        using chunk_handler_type = std::function<void(std::string_view, RouterContextT&)>;
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
            if (_complete_handler) {
                throw std::logic_error("init must be called() before complete()");
            }
            using traits = details::view_handler_traits<init_return_type, RouterContextT, HandlerT>;
            auto data = std::make_shared<typename traits::data_type>();
            _init_handler = [this, data, handler](const std::smatch& match, RouterContextT& ctx) {
                traits::load_data(match, *data);
                using invoker_type = typename traits::template invoker<decltype(handler)>;
                if constexpr (std::is_void_v<typename invoker_type::return_type>) {
                    invoker_type{}(handler, *data, ctx);
                    return init_return_type{ string_body{}, string_body::value_type{} };
                } else {
                    auto res = invoker_type{}(handler, *data, ctx);
                    return init_return_type{ body_traits::template body_of<decltype(res)>(), std::move(res) };
                }
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
            _complete_handler = [this, data, handler](const std::smatch& match, RouterContextT& ctx) -> return_type {
                traits::load_data(match, *data);
                using invoker_type = typename traits::template invoker<decltype(handler)>;
                auto result = std::move(invoker_type{}(handler, *data, ctx));
                using ResT = std::decay_t<decltype(result)>;
                if constexpr (std::disjunction_v<std::is_same<std::remove_cvref_t<ResT>, response<BodyTypes>>...>) {
                    return std::move(result);
                } else if constexpr (std::disjunction_v<std::is_same<std::remove_cvref_t<ResT>, typename BodyTypes::value_type>...>) {
                    response<typename body_traits::template body_of_value_t<ResT>> reply {
                        std::piecewise_construct,
                        std::make_tuple(std::move(result)),
                        std::make_tuple(status::ok, XDEV_NET_HTTP_DEFAULT_VERSION)
                    };
                    return std::move(reply);
                } else if constexpr (std::is_default_constructible_v<details::route_return_value<std::decay_t<ResT>>>) {
                    return details::route_return_value<std::decay_t<ResT>>::make_response(result);
                } else static_assert (always_false_v<ResT>, "Unhandled complete return type");
            };
            return *this;
        }

        void no_auto_regex() {
            auto tok = _path_escaped.find('<');
            tok = tok == std::string::npos ? tok : tok - 1;
            _regex = _path_escaped.substr(0, tok) + "/?(.+)?";
        }

        template <typename HandlerT>
        route& chunk(HandlerT handler) {
            using traits = details::view_handler_traits<void, RouterContextT, HandlerT>;
            _chunk_handler = [this, handler] (std::string_view data, RouterContextT& ctx) {
                if constexpr (traits::has_context_last) {
                    handler(data, ctx);
                } else {
                    handler(data);
                }
            };
            return *this;
        }

        std::tuple<bool, std::smatch> match(const std::string& url) {
            std::smatch match;
            return {std::regex_match(url, match, _regex), std::move(match)};
        }

        return_type do_complete(const std::smatch& match, RouterContextT& ctx) {
            return _complete_handler(match, ctx);
        }

        void do_init(const std::smatch& match, RouterContextT& ctx, parser_type& var, request_parser<empty_body>&req) {
            if (!_init_handler) {
                _assign_parser(var, req);
                return;
            } else {
                using parser_variant = typename body_traits::parser_variant;
                using body_value_variant = typename body_traits::body_value_variant;
                auto init_tup = _init_handler(match, ctx);
                std::visit([this,&var,&req, &init_tup](auto&body) {
                    using BodyT = std::decay_t<decltype(body)>;
                    std::visit(overloaded {
                       [] (auto&body_value) {
                            using BodyValueT = std::decay_t<decltype(body_value)>;
                            throw std::runtime_error("Body type mismatch: " +
                                       boost::typeindex::type_id<request_parser<BodyT>>().pretty_name() + " != " +
                                       boost::typeindex::type_id<BodyValueT>().pretty_name());
                       },
                       [this,&var,&req, &body] (typename BodyT::value_type& body_value) {
                           var.template emplace<request_parser<BodyT>>(std::move(req));
                           std::get<request_parser<BodyT>>(var).get().body() = std::move(body_value);
                       }
                    }, std::get<1>(init_tup));
                }, std::get<0>(init_tup));
            }
        }

        bool do_chunk(std::string_view data, const std::smatch& /*match*/, RouterContextT& ctx) {
            if (!_chunk_handler) {
                return false;
            }
            _chunk_handler(data, ctx);
            return true;
        }

    private:
        std::string _path, _path_escaped;
        request_parser_assign_func _assign_parser;
        std::regex _regex;
        handler_type _complete_handler;
        init_handler_type _init_handler;
        chunk_handler_type _chunk_handler;
    };

    route& add_route(const std::string& path) {
        return _routes.emplace_back(path);
    }

    std::tuple<route&, std::smatch> route_for(const std::string& url) {
        for (auto& route: _routes) {
            auto res = route.match(url);
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
