#pragma once

#include <net/socket.hpp>

namespace xdev::net {

struct udp_socket: socket
{
    udp_socket();
};

}  // namespace xdev::net
