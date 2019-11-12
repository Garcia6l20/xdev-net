#pragma once

#include <thread>
#include <memory>
#include <atomic>

namespace xdev {
struct thread_guard {
    thread_guard() = default;
    thread_guard(std::thread&& thread, std::shared_ptr<std::atomic_bool> running):
        _thread(std::move(thread)),
        _running(running){}
    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;
    thread_guard(thread_guard&& other) noexcept:
        _thread(std::move(other._thread)),
        _running(std::move(other._running)) {
    }
    thread_guard& operator=(thread_guard&& other) noexcept {
        _running = std::move(other._running);
        _thread = std::move(other._thread);
        return *this;
    }
    ~thread_guard() {
        (*this)();
    }
    void operator()() {
        if (!*this)
            return;
        *_running = false;
        _thread.join();
        _running.reset();
        _thread = std::thread();
    }
    operator bool() {
        return _running && _thread.joinable() && *_running;
    }
private:
    std::thread _thread;
    std::shared_ptr<std::atomic_bool> _running;
};
}
