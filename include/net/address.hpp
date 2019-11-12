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
        _family(other._family),
        _address(other._address),
        _port(other._port) {}
    address& operator=(const address& other) {
        _family = other._family;
        _address = other._address;
        _port = other._port;
        return *this;
    }
    address(const struct sockaddr* const addr);
    address(const std::string& hostname, uint16_t port, family family = inet);

    static address any() { return {"0.0.0.0", 0}; }

    operator struct sockaddr() const;

    uint32_t ip4_address() const {
        if (_family != family::inet)
            throw error("not an ipv4 address");
        return _address.ip4;
    }

private:
    family _family;
    union {
        uint32_t ip4;
    } _address;
    uint16_t _port;
};
}  // namespace xdev::net
