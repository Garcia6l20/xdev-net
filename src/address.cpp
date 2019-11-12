#include <net/error.hpp>
#include <net/address.hpp>

#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace xdev::net;

address::address(const std::string& hostname, uint16_t port, family family)
{
    hostent *he;
    if ((he = gethostbyname(hostname.c_str())) == nullptr)
        throw error(errno);

    in_addr **addr_list = reinterpret_cast<in_addr **>(he->h_addr_list);
    if (addr_list[0] == nullptr)
        throw error("cannot resolve hostname:" + hostname);

    _addr.sa_family = he->h_addrtype;
    switch (_addr.sa_family) {
    case family::inet:
        reinterpret_cast<sockaddr_in*>(&_addr)->sin_addr.s_addr = addr_list[0]->s_addr;
        reinterpret_cast<sockaddr_in*>(&_addr)->sin_port = port;
        break;
    case family::inet6:
        reinterpret_cast<sockaddr_in6*>(&_addr)->sin6_addr = *reinterpret_cast<in6_addr*>(addr_list[0]);
        reinterpret_cast<sockaddr_in6*>(&_addr)->sin6_port = port;
        break;
    }
}

address::address(const sockaddr* const addr):
    _addr{*addr} {
}

address::operator struct sockaddr() const {
    return _addr;
}

uint16_t address::port() const {
    switch (_addr.sa_family) {
    case family::inet:
        return reinterpret_cast<const sockaddr_in*>(&_addr)->sin_port;
    case family::inet6:
        return reinterpret_cast<const sockaddr_in6*>(&_addr)->sin6_port;
        break;
    }
}

std::string address::to_string() const {
    switch (_addr.sa_family) {
    case family::inet:
    case family::inet6: {
            std::string result(INET6_ADDRSTRLEN, 0);
            inet_ntop(_addr.sa_family, &_addr, result.data(), INET6_ADDRSTRLEN);
            result.resize(result.find(char{0}));
            result.append(":" + std::to_string(port()));
            return result;
        }
    default:
        throw error("unhandled family: " + std::to_string(_addr.sa_family));
    }
}
