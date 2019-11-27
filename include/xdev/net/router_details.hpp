#pragma once

#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <tuple>
#include <regex>
#include <variant>

#include <boost/beast/http.hpp>

#include <xdev/type_index.hpp>
#include <xdev/function_traits.hpp>

#include <xdev/net/route_parameter.hpp>

namespace xdev::net::http::details {


template <typename Ctx>
struct no_context_predicate {
    template<typename T>
    struct predicate {
        static constexpr bool value = true;
    };
};


template <typename ReturnT, typename ContextT, typename T>
struct view_handler_traits
    : public view_handler_traits<ReturnT, ContextT, decltype(&T::operator())>
{};

template <typename ReturnT, typename ContextT, typename ClassType, typename ReturnType, typename... Args>
struct view_handler_traits<ReturnT, ContextT, ReturnType(ClassType::*)(Args...) const> {

    using context_type = ContextT;
    using handler_traits = function_traits<ReturnType(ClassType::*)(Args...)>;
    using return_type = typename handler_traits::return_type;
    using function_type = typename handler_traits::function_type;
    static const size_t arity = handler_traits::arity;

    template <size_t idx>
    using arg = typename handler_traits::template arg<idx>;

    static constexpr bool has_context_last = arity > 0 && std::is_same<typename arg<arity - 1>::clean_type, context_type>::value;

    using no_context_predicate = typename handler_traits::template parameters_tuple_disable<context_type>;
    static_assert (no_context_predicate::template enabled<context_type> == false, "");

    using parameters_tuple_type = typename handler_traits::template parameters_tuple<no_context_predicate>;
    using data_type = typename parameters_tuple_type::tuple_type;

    static constexpr auto make_tuple = parameters_tuple_type::make;

    template<int idx, int match_idx, typename...ArgsT>
    struct _load_data;

    template<int type_idx, int match_idx, typename FirstT, typename...ArgsT>
    struct _load_data<type_idx, match_idx, FirstT, ArgsT...> {
        void operator()(data_type& data, const std::smatch& match) {
            using ParamT = std::remove_cvref_t<FirstT>;
            std::get<type_idx>(data) = route_parameter<ParamT>::load(match[match_idx].str());
            _load_data<type_idx + 1, match_idx + route_parameter<ParamT>::group_count() + 1, ArgsT...>{}(data, match);
        }
    };

    template<int type_idx, int match_idx, typename LastT>
    struct _load_data<type_idx, match_idx, LastT> {
        void operator()(data_type& data, const std::smatch& match) {
            using ParamT = std::remove_cvref_t<LastT>;
            if constexpr (!has_context_last)
                std::get<type_idx>(data) = route_parameter<ParamT>::load(match[match_idx].str());
        }
    };

    static bool load_data(const std::smatch& match, data_type& data) {
        if constexpr (sizeof...(Args))
            _load_data<0, 1, Args...>{}(data, match);
        return true;
    }

    template <int idx, typename Ret, typename Arg0, typename... ArgsT>
    static std::function<Ret(ArgsT...)> _make_recursive_lambda(std::function<Ret(Arg0, ArgsT...)> func, const data_type& data, context_type& ctx) {
        if constexpr (sizeof...(ArgsT) == 0 && has_context_last) {
            return [=, &ctx]() -> Ret {
                 return func(ctx);
            };
        } else {
            auto arg0 = std::get<idx>(data);
            return [=](ArgsT... args) -> Ret {
                 return func(arg0, args...);
            };
        }
    }


    template<typename Ret, typename Cls, typename IsMutable, typename IsLambda, typename... ArgsT>
    struct _invoker: function_detail::types<Ret, Cls, IsMutable, IsLambda, Args...> {
        template <int idx>
        static Ret _invoke (std::function<Ret()> func, const data_type& /*data*/, context_type& /*ctx*/)
        {
            return func();
        }

        template <int idx, typename Arg0, typename...RestArgsT>
        static Ret _invoke (std::function<Ret(Arg0, RestArgsT...)> func, const data_type& data, context_type& ctx)
        {
            return _invoke<idx + 1>(_make_recursive_lambda<idx>(func, data, ctx), data, ctx);
        }

        template <typename Fcn>
        Ret operator()(Fcn func, const data_type& data, context_type& ctx) {
            function_type fcn = func;
            return _invoke<0>(fcn, data, ctx);
        }
    };

    template<class T>
    struct invoker
        : invoker<decltype(&std::remove_cvref_t<T>::operator())>
    {};

    // mutable lambda
    template<class Ret, class Cls, class... ArgsT>
    struct invoker<Ret(Cls::*)(ArgsT...)>
        : _invoker<Ret, Cls, std::true_type, std::true_type, ArgsT...>
    {};

    // immutable lambda
    template<class Ret, class Cls, class... ArgsT>
    struct invoker<Ret(Cls::*)(ArgsT...) const>
        : _invoker<Ret, Cls, std::false_type, std::true_type, ArgsT...>
    {};

    // function
    template<class Ret, class... ArgsT>
    struct invoker<std::function<Ret(ArgsT...)>>
        : _invoker<Ret, std::nullptr_t, std::true_type, std::false_type, Args...>
    {};

    template <int idx>
    static void _make_match_pattern(std::string& pattern) {
        static const std::regex route_args_re(R"(<([^<>]+)>)");
        std::smatch match;
        if (!std::regex_search(pattern, match, route_args_re)) {
            throw std::exception();
        }
        pattern.replace(static_cast<size_t>(match.position()),
                        static_cast<size_t>(match.length()),
                        "(" + route_parameter<typename arg<idx>::clean_type>::pattern() + ")");
    }

    template <int idx, typename...ArgsT>
    struct pattern_maker;

    template <int idx, typename FirstT>
    struct pattern_maker<idx, FirstT> {
        void operator()(std::string& pattern) {
            if constexpr (!has_context_last)
                _make_match_pattern<idx>(pattern);
        }
    };

    template <int idx, typename FirstT, typename...RestT>
    struct pattern_maker<idx, FirstT, RestT...> {
        void operator()(std::string& pattern) {
            _make_match_pattern<idx>(pattern);
            pattern_maker<idx + 1, RestT...>{}(pattern);
        }
    };

    static std::regex make_regex(const std::string& path) {
        std::string pattern = path;
        if constexpr (sizeof...(Args))
            pattern_maker<0, Args...>{}(pattern);
        return std::regex(pattern);
    }

};  // class view_handler_traits

template <typename...BodyTypes>
struct body_traits {
    template <typename BodyType>
    using request = boost::beast::http::request<BodyType>;
    template <typename BodyType>
    using response = boost::beast::http::response<BodyType>;
    template <typename BodyType>
    using request_parser = boost::beast::http::request_parser<BodyType>;

    using body_variant = std::variant<BodyTypes...>;
    using body_value_variant = std::variant<typename BodyTypes::value_type...>;
    using response_variant = std::variant<response<BodyTypes>...>;
    using parser_variant = std::variant<request_parser<BodyTypes>...>;
    using request_variant = std::variant<request<BodyTypes>...>;

    template <typename T>
    static constexpr std::size_t body_index = type_index_v<T, BodyTypes...>;

    template <typename T>
    static constexpr auto body_value_index_v = type_index_v<std::remove_cvref_t<T>, typename BodyTypes::value_type...>;

    template <typename T>
    static constexpr auto body_index_v = type_index_v<std::remove_cvref_t<T>, BodyTypes...>;

    template <typename BodyValueType>
    static auto body_of() {
        return body_variant{std::in_place_index<body_value_index_v<BodyValueType>>};
    }

    template <typename BodyValueType>
    using body_of_value_t = type_at_t<type_index_v<std::remove_cvref_t<BodyValueType>, typename BodyTypes::value_type...>, BodyTypes...>;

};

}  // namespace xdev::net::details
