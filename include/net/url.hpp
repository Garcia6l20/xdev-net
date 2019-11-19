#pragma once

#include <regex>
#include <optional>

#include <net/error.hpp>

namespace xdev::net {

    /**
     * @brief split string
     */
    inline std::vector<std::string_view> split(const std::string_view& text, char sep, bool remove_empty = false)
    {
        std::vector<std::string_view> tokens;
        size_t start = 0, end = 0;
        std::string_view item;
        while ((end = text.find(sep, start)) != std::string::npos) {
            item = text.substr(start, end - start);
            if (!remove_empty || (remove_empty && !item.empty()))
            {
                tokens.push_back(item);
            }
            start = end + 1;
        }
        item = text.substr(start);
        if (!remove_empty || (remove_empty && !item.empty()))
        {
            tokens.push_back(item);
        }
        return tokens;
    }

    struct url {
        url(const std::string& data):
            _data{data} {
            static const std::regex url_exp{R"((\w+):(\/\/[\w:@]+)?(\/[^\?]+)?(\?([^#]+))?(\#\w+)?)"};
            std::smatch m;
            if (std::regex_match(_data, m, url_exp)) {
                _scheme = { _data.data() + m.position(1), static_cast<size_t>(m.length(1)) };
                _authority = { { _data.data() + m.position(2), static_cast<size_t>(m.length(2)) } };
                _path = { _data.data() + m.position(3), static_cast<size_t>(m.length(3)) };
                if (m[5].length()) {
                    std::string_view params_view = { _data.data() + m.position(5), static_cast<size_t>(m.length(5)) };
                    auto params = split(params_view, '&', true);
                    for (auto& param : params) {
                        auto s = split(param, '=', true);
                        _parameters.insert_or_assign(s[0], s[1]);
                    }
                }
                if (m[6].length() > 1) {
                    _fragment = { _data.data() + m.position(6) + 1, static_cast<size_t>(m.length(6)) - 1 };
                }
            } else {
                throw error("cannot parse url: " + _data);
            }
        }

        struct authority {
            authority() = default;
            authority(std::string_view view) {
                static const std::regex auth_exp{ R"(//((\w+)?:(\w+)?@)?(\w+)(:(\d+))?)" };
                std::smatch m;
                std::string tmp{ view };
                if (std::regex_match(tmp, m, auth_exp)) {
                    username = { view.data() + m.position(2), static_cast<size_t>(m.length(2)) };
                    password = { view.data() + m.position(3), static_cast<size_t>(m.length(3)) };
                    hostname = { view.data() + m.position(4), static_cast<size_t>(m.length(4)) };
                    if (m[6].length())
                        port = std::stoi(m[6]);
                } else {
                    throw error("cannot parse url's authority: " + tmp);
                }
            }
            std::string_view username;
            std::string_view password;
            std::string_view hostname;
            std::uint16_t port = 0;
        };

        std::string_view scheme() const {
            return _scheme;
        }

        std::string_view username() const {
            return _authority.username;
        }

        std::string_view password() const {
            return _authority.password;
        }

        std::string_view hostname() const {
            return _authority.hostname;
        }

        uint16_t port() const {
            return _authority.port;
        }

        std::string_view path() const {
            return _path;
        }

        std::string_view parameter(std::string_view key) const {
            return _parameters.at(key);
        }

        std::string_view fragment() const {
            return _fragment;
        }

    private:
        
        bool _valid = false;
        std::string _data;
        std::string_view _scheme;
        authority _authority;
        std::string_view _path;
        std::map<std::string_view, std::string_view> _parameters;
        std::string_view _fragment;
    };
}  // namespace xdev::net
