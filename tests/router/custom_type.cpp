#include <gtest/gtest.h>

#include <net/router.hpp>

using namespace xdev;
#if 1
struct TestType: std::pair<int, int> {
    using std::pair<int, int>::pair;
};

namespace xdev {
    template <>
    struct net::route_parameter<TestType> {
        static constexpr int group_count() { return 2; }
        static TestType load(const std::string& input) {
            static const std::regex re(pattern());
            std::smatch match;
            std::regex_match(input, match, re);
            return {stoi(match[1]), stoi(match[2])};
        }
        static std::string pattern() {
            return R"((\d+),(\d+))";
        }
    };
}

TEST(CustomTypeRouterTest, Nominal) {
    net::http::simple_router r;
    TestType result;
    auto view = [&](TestType value, net::http::simple_router::context_type&) -> net::http::simple_router::return_type {
        result = value;
    };
    using traits = net::http::details::view_handler_traits<net::http::simple_router::return_type, net::http::simple_router::context_type, decltype(view)>;
    auto test = net::http::simple_router::context_type();
    static_assert (traits::arity == 2, "");
    static_assert (std::is_same_v<traits::context_type, net::http::simple_router::context_type>, "");
    static_assert (std::is_same_v<traits::context_type, traits::arg<1>::clean_type>, "");
    static_assert (traits::has_context_last, "");
    r.add_route("/custom_type/<data>", view);
    r("/custom_type/42,55");
    ASSERT_EQ(result, TestType(42, 55));
}
#endif
