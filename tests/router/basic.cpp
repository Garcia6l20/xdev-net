#include <gtest/gtest.h>

#include <net/router.hpp>

using namespace xdev;

TEST(BasicRouterTest, HelloWorld) {
    net::http::simple_router r;
    std::string result;
    bool _friend = false;
    r.add_route("/hello/<who>", [&](std::string who) -> net::http::simple_router::return_type {
        result = who;
    });
    r.add_route("/hello/<who>/<is_friend>", [&](std::string who, bool is_friend) -> net::http::simple_router::return_type {
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
    ASSERT_THROW(r("/what"), net::http::simple_router::not_found);
}

TEST(BasicRouterTest, Add) {
    net::http::simple_router r;
    double result;
    r.add_route("/add/<left>/<right>", [&result](double left, double right) -> net::http::simple_router::return_type {
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
    using router_type = net::http::base_router<TestContext, std::void_t>;
    router_type r;
    r.add_route("/add/<left>/<right>", [](double left, double right, TestContext& ctx) -> router_type::return_type {
        ctx.result = left + right;
    });
    r("/add/20.2/21.8", ctx);
    ASSERT_EQ(ctx.result, 42);
}
