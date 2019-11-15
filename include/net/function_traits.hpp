#pragma once

#include <functional>

namespace xdev {

    namespace function_detail
    {
        template<typename Ret, typename Cls, typename IsMutable, typename IsLambda, typename... Args>
        struct types
        {
            using is_mutable = IsMutable;
            using is_lambda = IsLambda;
            static constexpr auto is_function() { return !is_lambda(); }

            enum { arity = sizeof...(Args) };

            using return_type = Ret;

            template<size_t i>
            struct arg
            {
                using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
            };
        };
    }

    template<class T>
    struct function_traits
        : function_traits<decltype(&std::remove_cvref_t<T>::operator())>
    {};

    // mutable lambda
    template<class Ret, class Cls, class... Args>
    struct function_traits<Ret(Cls::*)(Args...)>
        : function_detail::types<Ret, Cls, std::true_type, std::true_type, Args...>
    {};

    // immutable lambda
    template<class Ret, class Cls, class... Args>
    struct function_traits<Ret(Cls::*)(Args...) const>
        : function_detail::types<Ret, Cls, std::false_type, std::true_type, Args...>
    {};

    // function
    template<class Ret, class... Args>
    struct function_traits<std::function<Ret(Args...)>>
        : function_detail::types<Ret, std::nullptr_t, std::true_type, std::false_type, Args...>
    {};

}  // namespace xdev
