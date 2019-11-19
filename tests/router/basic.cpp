#include <gtest/gtest.h>

#include <net/router.hpp>

using namespace xdev;

TEST(BasicRouterTest, HelloWorld) {
    net::simple_router r;
    std::string result;
    bool _friend = false;
    r.add_route("/hello/<who>", [&](std::string who) {
        result = who;
    });
    r.add_route("/hello/<who>/<is_friend>", [&](std::string who, bool is_friend) {
        result = who;
        _friend = is_friend;
    });
    r("/hello/world");
    ASSERT_EQ(result, "world");
    ASSERT_FALSE(_friend);
    r("/hello/jhon/yes");
    ASSERT_EQ(result, "jhon");
    ASSERT_TRUE(_friend);
    r("/hello/jhon/no");
    ASSERT_EQ(result, "jhon");
    ASSERT_FALSE(_friend);
    ASSERT_THROW(r("/what"), net::simple_router::not_found);
}

TEST(BasicRouterTest, Add) {
    net::simple_router r;
    double result;
    r.add_route("/add/<left>/<right>", [&result](double left, double right) {
        result = left + right;
    });
    r("/add/20.2/21.8");
    ASSERT_EQ(result, 42);
}

struct TestContext {
    double result;
};

TEST(BasicRouterTest, Context) {
    TestContext ctx;
    net::base_router<void, TestContext> r;
    r.add_route("/add/<left>/<right>", [](double left, double right, TestContext& ctx) {
        ctx.result = left + right;
    });
    r("/add/20.2/21.8", ctx);
    ASSERT_EQ(ctx.result, 42);
}
