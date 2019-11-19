#include <gtest/gtest.h>

#include <router.hpp>

struct TestType: std::pair<int, int> {
    using std::pair<int, int>::pair;
};

namespace xdev {
    template <>
    struct route_parameter<TestType> {
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
    xdev::simple_router r;
    TestType result;
    auto view = [&](TestType value, const xdev::simple_router::context_type&) {
        result = value;
    };
    using traits = xdev::details::view_handler_traits<xdev::simple_router::return_type, xdev::simple_router::context_type, decltype (view)>;
    traits::context_type test = xdev::simple_router::context_type();
    static_assert (std::is_same<traits::context_type, xdev::simple_router::context_type>::value, "");
    //static_assert (!std::is_same<traits::context_type, traits::arg<1>::type>::value, "");
    static_assert (std::is_same<traits::context_type, traits::arg<1>::clean_type>::value, "");
    static_assert (traits::has_context_last, "");
    r.add_route("/custom_type/<data>", view);
    r("/custom_type/42,55");
    ASSERT_EQ(result, TestType(42, 55));
}
