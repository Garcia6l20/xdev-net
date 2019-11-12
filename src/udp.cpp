#include <net/udp.hpp>

using namespace xdev::net;

udp_socket::udp_socket():
    socket(family::inet, socket::dgram) {
}
