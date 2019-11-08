#include <net/socket.hpp>
#include <net/error.hpp>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace net;

socket::socket(family domain, type type, protocol protocol) {
    _fd = ::socket(domain, type, protocol);
    if (_fd < 0)
        throw error(errno);
}

socket::socket(int fd):
    _fd{fd} {
}

socket::~socket() {
    int fd = 0;
    std::swap(fd, _fd);
    if (fd > 0)
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
}

void socket::bind(const address& address) {
    struct sockaddr addr = address;
    if (::bind(_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        throw error(errno);
}

void socket::connect(const address &address) {
    struct sockaddr addr = address;
    if (::connect(_fd, &addr, sizeof(addr)) != 0)
        throw error(errno);
}

size_t socket::bytes_available(const clock::duration& timeout) const {
    if (timeout.count()) {
        struct timeval to_tv;
        auto const sec = std::chrono::duration_cast<std::chrono::seconds>(timeout);
        to_tv.tv_sec = sec.count();
        to_tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(timeout - sec).count();
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(_fd, &read_set);
        int res = select(FD_SETSIZE, &read_set, nullptr, nullptr, &to_tv);
        if (res < 0)
            throw error(errno);
        else if (res == 0)
            return 0;
    }
    size_t bytes_available = 0;
#ifdef _WIN32
        ioctlsocket(_fd, FIONREAD, &bytes_available);
#else
        ioctl(_fd, FIONREAD, &bytes_available);
#endif
        return bytes_available;
}

#include <iostream>

address socket::receive(void* buffer, size_t& buffer_sz) {
    struct sockaddr from {0, {0}};
    socklen_t from_len = sizeof(struct sockaddr);
    if (buffer_sz == 0) {
        throw error("buffer size is 0");
    } else {
        ssize_t n = ::recvfrom(_fd, buffer, buffer_sz, MSG_WAITALL, &from, &from_len);
        if (n < 0)
            throw error(errno);
        buffer_sz = static_cast<size_t>(n);
        if (from_len)
            return address(&from);
        else return address();
    }
}

void socket::send(const void* buffer, size_t sz, const address &to) {
    struct sockaddr add = to;
    ssize_t n = ::sendto(_fd, buffer, sz, MSG_CONFIRM, &add, sizeof(add));
    if (n < 0)
        throw error(errno);
}
