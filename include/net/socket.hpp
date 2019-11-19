#pragma once

#include <net/address.hpp>
#include <net/function_traits.hpp>

#include <functional>
#include <type_traits>
#include <chrono>
#include <future>
#include <iostream>
#include <optional>

#ifdef _HAS_JTHREAD // must wait for c++23
#include <jthread>
#else
#include <jthread.hpp>
#endif

namespace xdev::net {

struct buffer: std::vector<char> {
    using std::vector<char>::vector;
    buffer(const std::string& str):
        buffer(str.size()) {
        std::copy(str.begin(), str.end(), begin());
    }
    inline std::string_view string_view() const {
        return { data(), size() };
    }
    inline operator std::string_view() const {
        return string_view();
    }
};

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
template <typename T>
concept DataFromReceiver = requires(T a) {
    { a(buffer{}, net::address(sockaddr_storage{})) } -> void;
};
template <typename T>
concept DataReceiver = requires(T a) {
    { a(buffer{}) } -> void;
};
#else
#define DataContainer typename
#define DataFromReceiver typename
#define DataReceiver typename
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
#ifdef _WIN32
	using sock_fd_t = SOCKET;
#else
	using sock_fd_t = int;
#endif

    enum type {
        dgram = SOCK_DGRAM,
        stream = SOCK_STREAM,
    };

    enum protocol {
        none = 0
    };

    using clock = std::chrono::high_resolution_clock;

    socket(family family, type type, protocol protocol = none);
    socket(sock_fd_t fd);

    socket(const socket&) = delete;
    socket(socket&& other) noexcept:
        _fd(other._fd) {
        other._fd = 0;
    }

    virtual ~socket();

    void bind(const address& address);

    size_t bytes_available() const;
    bool wait_can_read(const clock::duration& timeout = std::chrono::milliseconds(0)) const;

    address receive(void* buffer, size_t& buffer_sz);

    struct pair_disconnected {};

    template <DataContainer ContainerT>
    std::optional<address> receive(ContainerT& buffer, const clock::duration &timeout = std::chrono::milliseconds(0)) {
        size_t sz = buffer.size();
        if (sz == 0) {
            bool can_read = false;
            if (timeout.count())
                can_read = wait_can_read(timeout);
            sz = bytes_available();
            if (sz == 0) {
                if (can_read)
                    throw pair_disconnected();
                return {};
            }
            buffer.resize(sz);
        }
        address&& from = receive(buffer.data(), sz);
        if (sz) {
            buffer.resize(sz);
            return std::move(from);
        }
        return {};
    }

    template <DataFromReceiver ReceiverT>
    [[nodiscard]] std::jthread
    start_receiver(ReceiverT&& receiver, std::function<void()> on_disconnect = nullptr) {
        return std::jthread([this, rx = std::forward<ReceiverT>(receiver), on_disconnect](std::stop_token&&stop) mutable {
            buffer data;
            while (!stop.stop_requested()) try {
                auto from = receive(data, std::chrono::milliseconds(25));
                if (from) {
                    rx(std::forward<buffer>(data), from.value());
                    data.clear();
                }
            } catch(const socket::pair_disconnected&) {
                if (on_disconnect)
                    on_disconnect();
            }
        });
    }

    template <DataReceiver ReceiverT>
    [[nodiscard]] std::jthread
    start_receiver(ReceiverT&& receiver, std::function<void()> on_disconnect = nullptr) {
        return start_receiver([receiver = std::forward<decltype(receiver)>(receiver)](buffer&& data, auto&& /*form*/) mutable {
            receiver(std::forward<buffer>(data));
        }, on_disconnect);
    }

    template <DataContainer ContainerT = std::vector<char>>
    [[nodiscard]] std::future<std::tuple<ContainerT, address>>
    receive(const clock::duration &timeout = std::chrono::milliseconds(0)) {
        return std::async([this, timeout]() -> std::tuple<ContainerT, address> {
            ContainerT data;
            bool can_read = false;
            if (timeout.count())
                can_read = wait_can_read(timeout);
            size_t bytes = bytes_available();
            if (bytes == 0) {
                if (can_read)
                    throw pair_disconnected();
                return {data, {}};
            }
            data.resize(bytes);
            auto from = receive(data);
            if (!from)
                throw error("reveive failed");
            return {data, from.value()};
        });
    }

    void send(const void* buffer, size_t buffer_sz) const;
    void send(const void* buffer, size_t buffer_sz, const address& to) const;

    template <DataContainer ContainerT>
    void send(const ContainerT& buffer, const address& to) const {
        send(buffer.data(), buffer.size(), to);
    }

    template <DataContainer ContainerT>
    void send(const ContainerT& buffer) const {
        send(buffer.data(), buffer.size());
    }

    template <SocketAcceptor AcceptFunctorT>
    [[nodiscard]] std::jthread
    listen(AcceptFunctorT&& functor, int max_conn = 100) {
        std::promise<void> started_promise;
        auto fut = started_promise.get_future();
        auto th = std::jthread([this, f = std::forward<AcceptFunctorT>(functor), max_conn, started_promise=std::move(started_promise)](std::stop_token&& stop) mutable {
            sockaddr_storage peer_add;
            socklen_t peer_add_sz = sizeof(peer_add);
            struct timeval to_tv;
            to_tv.tv_sec = 0;
            to_tv.tv_usec = 25000;
            fd_set read_set;
            if (::listen(_fd, max_conn) != 0)
                throw error(_socket_error());
            started_promise.set_value();
            while (!stop.stop_requested()) {
                FD_ZERO(&read_set);
                FD_SET(_fd, &read_set);
                int res = ::select(FD_SETSIZE, &read_set, nullptr, nullptr, &to_tv);
                if (res < 0)
                    throw error(_socket_error());
                else if (res == 0)
                    continue;
                auto conn_fd = ::accept(_fd, reinterpret_cast<sockaddr*>(&peer_add), &peer_add_sz);
                if (conn_fd < 0)
                    throw error(_socket_error());
                f(socket{conn_fd}, address{peer_add});
            }
        });
        fut.get();
        return th;
    }

    void connect(const address& address);

	sock_fd_t fd() const { return _fd; }

private:
	sock_fd_t _fd;
};

}  // namespace xdev::net
