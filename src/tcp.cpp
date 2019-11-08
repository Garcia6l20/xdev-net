#include <net/tcp.hpp>

using namespace net;

tcp_client::tcp_client():
    socket(family::inet, socket::stream) {
}

tcp_client::~tcp_client() {}
