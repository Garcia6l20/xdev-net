#include <gtest/gtest.h>

#include <xdev/function_traits.hpp>

#include <iostream>

using namespace std::literals::string_literals;
using namespace xdev;

TEST(FunctionTraits, Function) {
    std::function func = [](int, std::string) {
        return 0;
    };
    using func_traits = function_traits<decltype(func)>;
    ASSERT_EQ(func_traits::arity, 2);
    ASSERT_EQ(func_traits::return_type{}, int{});
    ASSERT_EQ(func_traits::arg<0>::type{}, int{});
    ASSERT_EQ(func_traits::arg<1>::type{}, std::string{});
    ASSERT_TRUE(func_traits::is_mutable());
    ASSERT_TRUE(func_traits::is_function());
}

TEST(FunctionTraits, Lambda) {
    auto func = [](int, std::string) {
        return 0;
    };
    using func_traits = function_traits<decltype(func)>;
    ASSERT_EQ(func_traits::arity, 2);
    ASSERT_EQ(func_traits::return_type{}, int{});
    ASSERT_EQ(func_traits::arg<0>::type{}, int{});
    ASSERT_EQ(func_traits::arg<1>::type{}, std::string{});
    ASSERT_FALSE(func_traits::is_mutable());
    ASSERT_TRUE(func_traits::is_lambda());
}

TEST(FunctionTraits, LambdaMutable) {
    auto func = [](int, std::string) mutable {
        return 0;
    };
    using func_traits = function_traits<decltype(func)>;
    ASSERT_EQ(func_traits::arity, 2);
    ASSERT_EQ(func_traits::return_type{}, int{});
    ASSERT_EQ(func_traits::arg<0>::type{}, int{});
    ASSERT_EQ(func_traits::arg<1>::type{}, std::string{});
    ASSERT_TRUE(func_traits::is_mutable());
    ASSERT_TRUE(func_traits::is_lambda());
}
