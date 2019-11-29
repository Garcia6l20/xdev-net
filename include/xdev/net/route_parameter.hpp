#pragma once

#include <algorithm>
#include <string>
#include <filesystem>
#include <vector>

#include <boost/beast/http.hpp>

#include <xdev/net/config.hpp>

namespace xdev::net::http::details {

template <typename T>
struct route_parameter;

template <>
struct route_parameter<std::string> {
    static constexpr int group_count() { return 0; }
    static std::string load(const std::string& input) {
        return input;
    }
    static std::string pattern() {
        return R"([^/]+)";
    }
};

template <>
struct route_parameter<std::filesystem::path> {
    static constexpr int group_count() { return 0; }
    static std::filesystem::path load(const std::string& input) {
        return std::filesystem::path(input.c_str());
    }
    static std::string pattern() {
        return R"(.+)";
    }
};

template <>
struct route_parameter<int> {
    static constexpr int group_count() { return 0; }
    static int load(const std::string& input) {
        return stoi(input);
    }
    static std::string pattern() {
        return R"(\d+)";
    }
};

template <>
struct route_parameter<double> {
    static constexpr int group_count() { return 0; }
    static double load(const std::string& input) {
        return stod(input);
    }
    static std::string pattern() {
        return R"(\d+\.?\d*)";
    }
};

template <>
struct route_parameter<bool> {
    static bool load(const std::string& input) {
        static const std::vector<std::string> true_values = {"yes", "on", "true"};
        return std::any_of(true_values.begin(), true_values.end(), [&input](auto&&elem) {
            return std::equal(elem.begin(), elem.end(), input.begin(), input.end(), [](char left, char right) {
               return tolower(left) == tolower(right);
            });
        });
    }
    static std::string pattern() {
        return R"(\w+)";
    }
};

namespace http = boost::beast::http;

template <typename T>
struct route_return_value;

template <>
struct route_return_value<std::string> {
    static auto make_response(const std::string& result) {
        http::response<http::string_body> reply{
            std::piecewise_construct,
            std::make_tuple(result),
            std::make_tuple(http::status::ok, XDEV_NET_HTTP_DEFAULT_VERSION)
        };
        reply.set(http::field::content_type, "text/plain");
        reply.content_length(reply.body().size());
        return reply;
    }
};

template <>
struct route_return_value<std::tuple<std::string, std::string>> {
    static auto make_response(const std::tuple<std::string, std::string>& result) {
        http::response<http::string_body> reply{
            std::piecewise_construct,
            std::make_tuple(std::get<1>(result)),
            std::make_tuple(http::status::ok, XDEV_NET_HTTP_DEFAULT_VERSION)
        };
        reply.set(http::field::content_type, std::get<0>(result));
        reply.content_length(reply.body().size());
        return reply;
    }
};

template <>
struct route_return_value<std::tuple<http::status, std::string, std::string>> {
    static auto make_response(const std::tuple<http::status, std::string, std::string>& result) {
        http::response<http::string_body> reply{
            std::piecewise_construct,
                std::make_tuple(std::get<2>(result)),
                std::make_tuple(std::get<0>(result), XDEV_NET_HTTP_DEFAULT_VERSION)
        };
        reply.set(http::field::content_type, std::get<1>(result));
        reply.content_length(reply.body().size());
        return reply;
    }
};

}  // namespace xdev::net
