#include <gtest/gtest.h>

#include <xdev/type_index.hpp>

#include <iostream>

using namespace std::literals::string_literals;
using namespace xdev;

template <typename...Ts>
struct test_index_of {
    template <typename T>
    static constexpr auto index_of = type_index_v<T, Ts...>;

    template <size_t index>
    using type_at = type_at_t<index, Ts...>;
};

TEST(IndexOf, Nominal) {
    using test_t = test_index_of<bool, int, double>;
    ASSERT_EQ(0, test_t::index_of<bool>);
    ASSERT_EQ(1, test_t::index_of<int>);
    ASSERT_EQ(2, test_t::index_of<double>);
}

struct custom_type {};

TEST(IndexOf, CustomType) {
    using test_t = test_index_of<bool, int, double, custom_type>;
    ASSERT_EQ(0, test_t::index_of<bool>);
    ASSERT_EQ(1, test_t::index_of<int>);
    ASSERT_EQ(2, test_t::index_of<double>);
    ASSERT_EQ(3, test_t::index_of<custom_type>);
}

TEST(TypeAt, Nominal) {
    using test_t = test_index_of<bool, int, double>;
    ASSERT_EQ(typeid(test_t::type_at<0>), typeid(bool));
    ASSERT_EQ(typeid(test_t::type_at<1>), typeid(int));
    ASSERT_EQ(typeid(test_t::type_at<2>), typeid(double));
}

TEST(TypeAt, CustomType) {
    using test_t = test_index_of<bool, int, double, custom_type>;
    ASSERT_EQ(typeid(test_t::type_at<0>), typeid(bool));
    ASSERT_EQ(typeid(test_t::type_at<1>), typeid(int));
    ASSERT_EQ(typeid(test_t::type_at<2>), typeid(double));
    ASSERT_EQ(typeid(test_t::type_at<3>), typeid(custom_type));
}