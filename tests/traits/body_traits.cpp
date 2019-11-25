#include <gtest/gtest.h>

#include <xdev/net/router_details.hpp>

using namespace xdev;

TEST(IndexOf, Nominal) {
    using body_traits = net::http::details::body_traits<boost::beast::http::empty_body, boost::beast::http::file_body>;
    ASSERT_EQ(typeid(body_traits::body_variant), typeid(body_traits::body_of<boost::beast::http::file_body::value_type>()));

    auto var = body_traits::body_of<boost::beast::http::file_body::value_type>();
    ASSERT_EQ(var.index(), 1);
}
