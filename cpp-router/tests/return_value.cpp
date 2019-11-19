#include <gtest/gtest.h>

#include <router.hpp>

TEST(RouterTest, ReturnValue) {
    xdev::base_router<int> r;
    r.add_route("/return/<data>", [](int value) {
        return value;
    });
    ASSERT_EQ(r("/return/42"), 42);
}

TEST(RouterTest, Add) {
    xdev::base_router<int> r;
    r.add_route("/add/<left>/<right>", [](double left, double right) {
        return left + right;
    });
    ASSERT_EQ(r("/add/20.2/21.8"), 42);
}
