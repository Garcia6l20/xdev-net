#include <gtest/gtest.h>

#include <net/router.hpp>

using namespace xdev;

TEST(RouterTest, ReturnValue) {
    net::http::base_router<int> r;
    r.add_route("/return/<data>", [](int value) {
        return value;
    });
    ASSERT_EQ(r("/return/42"), 42);
}

TEST(RouterTest, Add) {
    net::http::base_router<int> r;
    r.add_route("/add/<left>/<right>", [](double left, double right) -> int {
        return left + right;
    });
    ASSERT_EQ(r("/add/20.2/21.8"), 42);
}
