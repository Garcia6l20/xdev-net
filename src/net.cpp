#include <net/net.hpp>

#include <string>
#include <stdexcept>

namespace xdev::net {

void initialize() {
#ifdef _WIN32
	// Initialize Winsock
	WSADATA wsaData;
	if (int res = WSAStartup(MAKEWORD(2, 2), &wsaData); res) {
		throw std::runtime_error("WSAStartup failed: " + std::to_string(res));
	}
#endif
}

void cleanup() {
#ifdef _WIN32
	WSACleanup();
#endif // _WIN32
}

}
