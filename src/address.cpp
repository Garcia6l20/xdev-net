#include <net/error.hpp>
#include <net/address.hpp>

#include <cstring>

#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace xdev::net;

address::address(const std::string& hostname, uint16_t port, enum family family)
{
    hostent *he;
    if ((he = gethostbyname2(hostname.c_str(), family)) == nullptr)
        throw error(errno);

    in_addr **addr_list = reinterpret_cast<in_addr **>(he->h_addr_list);
    if (addr_list[0] == nullptr)
        throw error("cannot resolve hostname:" + hostname);

    raw().sa_family = static_cast<sa_family_t>(he->h_addrtype);
    switch (this->family()) {
    case family::inet:
        reinterpret_cast<sockaddr_in*>(&_addr)->sin_addr.s_addr = addr_list[0]->s_addr;
        reinterpret_cast<sockaddr_in*>(&_addr)->sin_port = ::htons(port);
        break;
    case family::inet6:
        reinterpret_cast<sockaddr_in6*>(&_addr)->sin6_addr = *reinterpret_cast<in6_addr*>(addr_list[0]);
        reinterpret_cast<sockaddr_in6*>(&_addr)->sin6_port = ::htons(port);
        break;
    default:
        throw error("unhandled family: " + std::to_string(raw().sa_family));
    }
}

address::address(const sockaddr_storage& addr):
    _addr{addr} {
}

address::operator struct sockaddr() const {
    return raw();
}

uint16_t address::port() const {
    switch (family()) {
    case family::inet:
        return ::ntohs(reinterpret_cast<const sockaddr_in*>(&_addr)->sin_port);
    case family::inet6:
        return ::ntohs(reinterpret_cast<const sockaddr_in6*>(&_addr)->sin6_port);
    default:
        throw error("unhandled family: " + std::to_string(raw().sa_family));
    }
}

family address::family() const {
    return static_cast<enum family>(raw().sa_family);
}

std::string address::to_string() const {
    switch (family()) {
    case family::inet: {
            std::string result(INET_ADDRSTRLEN, 0);
            inet_ntop(raw().sa_family, &reinterpret_cast<const sockaddr_in*>(&_addr)->sin_addr.s_addr, result.data(), INET_ADDRSTRLEN);
            result.resize(result.find(char{0}));
            result.append(":" + std::to_string(port()));
            return result;
        }
    case family::inet6: {
            std::string result(INET6_ADDRSTRLEN, 0);
            inet_ntop(raw().sa_family, &reinterpret_cast<const sockaddr_in6*>(&_addr)->sin6_addr, result.data(), INET6_ADDRSTRLEN);
            result.resize(result.find(char{0}));
            result.append(":" + std::to_string(port()));
            return result;
        }
    default:
        throw error("unhandled family: " + std::to_string(raw().sa_family));
    }
}

bool address::operator==(const address& other) const {
    if (family() != other.family())
        return false;
    switch (family()) {
    case family::inet: {
            return !std::memcmp(&_addr, &other._addr, sizeof(sockaddr_in));
        }
    case family::inet6: {
            return !std::memcmp(&_addr, &other._addr, sizeof(sockaddr_in6));
        }
    default:
        throw error("unhandled family: " + std::to_string(raw().sa_family));
    }
}
