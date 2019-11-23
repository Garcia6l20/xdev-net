#include <gtest/gtest.h>

#include <xdev/net/url.hpp>

#include <iostream>

using namespace std::literals::string_literals;
using namespace xdev;

TEST(Url, Nominal) {
    net::url url{"http://user:pass@localhost:4242/path?test1=1&test2=2#42"};
    ASSERT_EQ("http", url.scheme());
    ASSERT_EQ("user", url.username());
    ASSERT_EQ("pass", url.password());
    ASSERT_EQ("localhost", url.hostname());
    ASSERT_EQ(4242, url.port());
    ASSERT_EQ("/path", url.path());
    ASSERT_EQ("1", url.parameter("test1"));
    ASSERT_EQ("2", url.parameter("test2"));
    ASSERT_EQ("42", url.fragment());
}

TEST(Url, Simple) {
    net::url url{ "http://localhost" };
    ASSERT_EQ("http", url.scheme());
    ASSERT_TRUE(url.username().empty());
    ASSERT_TRUE(url.password().empty());
    ASSERT_EQ("localhost", url.hostname());
    ASSERT_EQ(0, url.port());
    ASSERT_TRUE(url.path().empty());
    ASSERT_THROW(url.parameter("test1"), std::out_of_range);
    ASSERT_THROW(url.parameter("test2"), std::out_of_range);
    ASSERT_TRUE(url.fragment().empty());
}
