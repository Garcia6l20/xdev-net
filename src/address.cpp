#include <net/error.hpp>
#include <net/address.hpp>

#ifndef _WIN32
#include <netdb.h>
#include <arpa/inet.h>
#endif

using namespace net;

address::address(const std::string& hostname, uint16_t port, family family):
    _family(family),
    _port(port)
{
    struct hostent *he;
    if ((he = gethostbyname(hostname.c_str())) == nullptr)
        throw error(errno);

    struct in_addr **addr_list = reinterpret_cast<struct in_addr **>(he->h_addr_list);
    if (addr_list[0] == nullptr)
        throw error("cannot resolve hostname:" + hostname);
    for (int ii = 0; addr_list[ii] != nullptr; ++ii) {
        _address.ip4 = addr_list[ii]->s_addr;
    }
}

address::address(const sockaddr* const add):
    _family(static_cast<family>(add->sa_family)){
    switch (_family) {
    case family::inet:
        _address.ip4 = reinterpret_cast<const struct sockaddr_in*>(add)->sin_addr.s_addr;
        _port = reinterpret_cast<const struct sockaddr_in*>(add)->sin_port;
        break;
    default:
        throw error("unhandled family: " + std::to_string(_family));
    }
}

address::operator struct sockaddr() const {
    switch (_family) {
    case family::inet:
        sockaddr add;
        reinterpret_cast<sockaddr_in*>(&add)->sin_family = _family;
        reinterpret_cast<sockaddr_in*>(&add)->sin_addr.s_addr = _address.ip4;
        reinterpret_cast<sockaddr_in*>(&add)->sin_port = _port;
        return add;
    default:
        throw error("unhandled family: " + std::to_string(_family));
    }
}
