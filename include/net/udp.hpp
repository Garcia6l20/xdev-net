#pragma once

#include <net/socket.hpp>

namespace net {

struct udp_socket: socket
{
    udp_socket();
};

}  // namespace net
