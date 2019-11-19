#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <router_details.hpp>

namespace xdev {

struct _dumb_route_context {
};

template <typename RouteRetT = void, typename RouterContextT = _dumb_route_context>
class base_router
{
public:
    using return_type = RouteRetT;
    using context_type = RouterContextT;

    struct not_found: std::exception {
    };

    base_router() = default;

    template <typename ViewHandler>
    void add_route(const std::string& path, ViewHandler view_handler) {
        _routes.push_back({path, view_handler});
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
        route(const std::string& path, ViewHandler handler) {
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

        RouteRetT operator()(const std::string& url, RouterContextT& ctx) {
            std::smatch match;
            if (std::regex_match(url, match, _regex)) {
                return _handler(match, ctx);
            }
            throw not_matching();
        }

    private:
        std::regex _regex;
        handler_type _handler;
    };

    std::vector<route> _routes;
};

using simple_router = base_router<>;

} // namespace xdev

#endif // ROUTER_HPP
