#pragma once

#include <net/net.hpp>
#include <net/error.hpp>

#include <string>

#ifndef _WIN32
#include <netinet/in.h>
#endif

namespace xdev::net {
struct address {
    address() = default;
    address(const address& other):
        _addr(other._addr) {}
    address& operator=(const address& other) {
        _addr = other._addr;
        return *this;
    }
    address(const struct sockaddr* const addr);
    address(const std::string& hostname, uint16_t port, family family = inet);

    static address any() { return {"0.0.0.0", 0}; }

    operator struct sockaddr() const;

    uint16_t port() const;

    uint32_t ip4_address() const {
        if (_addr.sa_family != family::inet)
            throw error("not an ipv4 address");
        return reinterpret_cast<const sockaddr_in*>(&_addr)->sin_addr.s_addr;
    }

    std::string to_string() const;

    friend std::ostream& operator<< (std::ostream& stream, const address& address) {
        stream << address.to_string();
        return stream;
    }

private:
    sockaddr _addr;
};
}  // namespace xdev::net

namespace std {
inline std::string to_string(const xdev::net::address& add) {
    return add.to_string();
}
}
