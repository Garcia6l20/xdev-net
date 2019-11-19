#pragma once

#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <tuple>
#include <regex>

#include <net/route_parameter.hpp>
#include <net/function_traits.hpp>

namespace xdev::net::details {

template <typename ReturnT, typename ContextT, typename T>
struct view_handler_traits
    : public view_handler_traits<ReturnT, ContextT, decltype(&T::operator())>
{};

template <typename ReturnT, typename ContextT, typename ClassType, typename ReturnType, typename... Args>
struct view_handler_traits<ReturnT, ContextT, ReturnType(ClassType::*)(Args...) const> {

    using traits = function_traits<ReturnType(ClassType::*)(Args...)>;
    using return_type = typename traits::return_type;
    using function_type = typename traits::function_type;
    static const size_t arity = traits::arity;

    template <size_t idx>
    using arg = typename traits::template arg<idx>;

    static_assert (std::is_same<ReturnT, ReturnType>::value, "Bad route return type");

    using context_type = ContextT;

    template <typename FirstT, typename...RestT>
    static constexpr auto __make_tuple() {
        if constexpr (!sizeof...(RestT)){
            if constexpr (has_context_last)
                return std::make_tuple<bool>({true});
            else return std::make_tuple<FirstT>({});
        } else {
            return std::tuple_cat(std::make_tuple<FirstT>({}), __make_tuple<RestT...>());
        }
    }

    struct _make_tuple {
        constexpr auto operator()() {
            return __make_tuple<Args...>();
        }
    };

    static constexpr auto make_tuple() {
        return _make_tuple{}();
    }

    using data_type = std::invoke_result_t<_make_tuple>;

    static constexpr bool has_context_last = std::is_same<typename arg<arity - 1>::clean_type, context_type>::value;
    //static_assert (has_context_last, "");

    template<int idx, int match_idx, typename...ArgsT>
    struct _load_data;

    template<int type_idx, int match_idx, typename FirstT, typename...ArgsT>
    struct _load_data<type_idx, match_idx, FirstT, ArgsT...> {
        void operator()(data_type& data, const std::smatch& match) {
            std::get<type_idx>(data) = route_parameter<FirstT>::load(match[match_idx].str());
            _load_data<type_idx + 1, match_idx + route_parameter<FirstT>::group_count() + 1, ArgsT...>{}(data, match);
        }
    };

    template<int type_idx, int match_idx, typename LastT>
    struct _load_data<type_idx, match_idx, LastT> {
        void operator()(data_type& data, const std::smatch& match) {
            if constexpr (!has_context_last)
                std::get<type_idx>(data) = route_parameter<LastT>::load(match[match_idx].str());
        }
    };

    static bool load_data(const std::smatch& match, data_type& data) {
        _load_data<0, 1, Args...>{}(data, match);
        return true;
    }

    template <int idx, typename Ret, typename Arg0, typename... ArgsT>
    static std::function<Ret(ArgsT...)> _make_recursive_lambda(std::function<Ret(Arg0, ArgsT...)> func, const data_type& data, context_type& ctx) {
        if constexpr (sizeof...(ArgsT) == 0 && has_context_last) {
            static_assert (std::is_same_v<std::remove_cv_t<std::remove_reference_t<Arg0>>, context_type>, "Ooops");
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

    template <int idx, typename Ret>
    static Ret _invoke (std::function<Ret()> func, const data_type& /*data*/, context_type& /*ctx*/)
    {
        return func();
    }

    template <int idx, typename Ret, typename Arg0, typename...ArgsT>
    static Ret _invoke (std::function<Ret(Arg0, ArgsT...)> func, const data_type& data, context_type& ctx)
    {
        return _invoke<idx + 1>(_make_recursive_lambda<idx>(func, data, ctx), data, ctx);
    }

    static return_type invoke(function_type func, const data_type& data, context_type& ctx) {
        return _invoke<0>(func, data, ctx);
    }

    template <int idx>
    static void _make_match_pattern(std::string& pattern) {
        static const std::regex route_args_re(R"(<([^<>]+)>)");
        std::smatch match;
        if (!std::regex_search(pattern, match, route_args_re)) {
            throw std::exception();
        }
        pattern.replace(static_cast<size_t>(match.position()),
                        static_cast<size_t>(match.length()),
                        "(" + route_parameter<typename arg<idx>::type>::pattern() + ")");
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
        static const std::regex route_args_re(R"(<([^<>]+)>)");
        std::string pattern = path;
        pattern_maker<0, Args...>{}(pattern);
        return std::regex(pattern);
    }

};  // class view_handler_traits

}  // namespace xdev::net::details
