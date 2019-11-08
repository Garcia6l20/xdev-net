#include <net/udp.hpp>

using namespace net;

udp_socket::udp_socket():
    socket(family::inet, socket::dgram) {
}
