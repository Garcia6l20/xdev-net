#pragma once

#include <net/thread_guard.hpp>
#include <net/address.hpp>

#include <functional>
#include <type_traits>
#include <chrono>
#include <future>
#include <iostream>
#include <optional>

namespace xdev::net {

struct socket;

#if defined(__cpp_concepts)
template <typename T>
concept DataContainer = requires(T a) {
    { a.data() } -> const char*;
    { a.size() } -> size_t;
    { a.resize(size_t(0)) } -> void;
};
template <typename T>
concept SocketAcceptor = requires(T a) {
    { a(net::socket(-1), net::address(sockaddr_storage{})) } -> void;
};
#else
#define DataContainer typename
#define SocketAcceptor typename
#endif

inline auto _socket_error() {
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

struct socket
{
    enum type {
        dgram = SOCK_DGRAM,
        stream = SOCK_STREAM,
    };

    enum protocol {
        none = 0
    };

    using clock = std::chrono::high_resolution_clock;

    socket(family family, type type, protocol protocol = none);
    socket(int fd);

    socket(const socket&) = delete;
    socket(socket&& other) noexcept:
        _fd(other._fd) {
        other._fd = 0;
    }

    virtual ~socket();

    void bind(const address& address);

    size_t bytes_available(const clock::duration &timeout = std::chrono::milliseconds(0)) const;

    address receive(void* buffer, size_t& buffer_sz);

    template <DataContainer ContainerT>
    std::optional<address> receive(ContainerT& buffer, const clock::duration &timeout = std::chrono::milliseconds(0)) {
        size_t sz = buffer.size();
        if (sz == 0) {
            sz = bytes_available(timeout);
            if (sz == 0)
                return {};
            buffer.resize(sz);
        }
        address&& from = receive(buffer.data(), sz);
        if (sz) {
            buffer.resize(sz);
            return std::move(from);
        }
        return {};
    }

    template <typename ReceiverT>
    [[nodiscard]] thread_guard
    start_receiver(ReceiverT&& receiver) {
        auto running = std::make_shared<std::atomic_bool>(true);
        return thread_guard{std::thread([this, rx = std::forward<ReceiverT>(receiver), running]() mutable {
            std::vector<char> data;
            while (*running) {
                auto from = receive(data, std::chrono::milliseconds(25));
                if (from) {
                    rx({data.data(), data.size()}, from.value());
                    data.clear();
                }
            }
        }), running};
    }

    template <DataContainer ContainerT = std::vector<char>>
    [[nodiscard]] std::future<std::tuple<ContainerT, address>>
    receive(const clock::duration &timeout = std::chrono::milliseconds(0)) {
        return std::async([this, timeout] {
            size_t bytes = bytes_available(timeout);
            if (!bytes)
                throw error("timeout");
            ContainerT data;
            data.resize(bytes);
            auto from = receive(data);
            if (!from)
                throw error("reveive failed");
            return std::tuple{data, from.value()};
        });
    }

    void send(const void* buffer, size_t buffer_sz, const address& to);

    template <DataContainer ContainerT>
    void send(const ContainerT& buffer, const address& to) {
        send(buffer.data(), buffer.size(), to);
    }

    template <SocketAcceptor AcceptFunctorT>
    [[nodiscard]] thread_guard
    listen(AcceptFunctorT&& functor, int max_conn = 100) {
        auto running = std::make_shared<std::atomic<bool>>(true);
        return thread_guard{std::thread([this, f = std::forward<AcceptFunctorT>(functor), running, max_conn]() mutable {
            sockaddr_storage peer_add;
            socklen_t peer_add_sz = sizeof(peer_add);
            struct timeval to_tv;
            to_tv.tv_sec = 0;
            to_tv.tv_usec = 25000;
            fd_set read_set;
            if (::listen(_fd, max_conn) != 0)
                throw error(_socket_error());
            while (*running) {
                FD_ZERO(&read_set);
                FD_SET(_fd, &read_set);
                int res = ::select(FD_SETSIZE, &read_set, nullptr, nullptr, &to_tv);
                if (res < 0)
                    throw error(_socket_error());
                else if (res == 0)
                    continue;
                int conn_fd = ::accept(_fd, reinterpret_cast<sockaddr*>(&peer_add), &peer_add_sz);
                if (conn_fd < 0)
                    throw error(_socket_error());
                f(socket{conn_fd}, address{peer_add});
            }
        }), running};
    }

    void connect(const address& address);

    int fd() const { return _fd; }

private:
    int _fd;
};

}  // namespace xdev::net
