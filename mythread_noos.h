// mythread_noos.h
// Tiny "no-OS-threads" compatibility layer.
// This file provides a Thread/Mutex/LockGuard/CondVar API but does not use any OS threads.
// All Thread::start(...) will execute the callable synchronously on the caller thread.
// Purpose: allow code that expects threads to compile & run even when <thread> and pthreads are unavailable.
//
// NOTE: This is a *fallback* for environments where real threads are impossible.
// It preserves API shape & logging, but there is NO concurrency.

#ifndef MYTHREAD_NOOS_H
#define MYTHREAD_NOOS_H

#include <functional>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <string>

namespace MyThreadNoOS {

using MilliClock = std::chrono::high_resolution_clock;
using ms = std::chrono::duration<double, std::milli>;

// Dummy Mutex: no-op (everything runs on same thread)
class Mutex {
public:
    Mutex() {}
    ~Mutex() {}
    void lock() {}
    void unlock() {}
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    // Provide a native_handle() placeholder type to satisfy some code that expects it.
    void* native_handle() { return nullptr; }
};

// Lock guard that uses the dummy Mutex
class LockGuard {
public:
    explicit LockGuard(Mutex &m) : mtx_(m) { mtx_.lock(); }
    ~LockGuard() { mtx_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
private:
    Mutex &mtx_;
};

// Minimal condition variable-like API that evaluates the predicate immediately.
// wait(m, pred): if pred() is false, it returns immediately (since no other thread will notify).
class CondVar {
public:
    CondVar() {}
    ~CondVar() {}
    void notify_one() {}
    void notify_all() {}
    template<typename Predicate>
    void wait(Mutex &m, Predicate pred) {
        // since we run synchronously and there are no other threads,
        // just check predicate once and return.
        (void)m;
        (void)pred;
    }

    template<typename Predicate>
    bool wait_for(Mutex &m, long long ms_timeout, Predicate pred) {
        (void)m; (void)ms_timeout;
        // evaluate predicate once and return result
        return pred();
    }
};

// Thread: synchronous "fake" thread. start(...) runs the callable immediately and stores timing info.
// The interface mimics a minimal subset of std::thread.
class Thread {
public:
    Thread() : started_(false), joined_(false) {}
    // Construct-and-start
    template<typename Callable>
    explicit Thread(Callable&& c) : started_(false), joined_(false) {
        start(std::forward<Callable>(c));
    }

    template<typename Callable>
    void start(Callable&& c) {
        if (started_) throw std::logic_error("Thread already started");
        started_ = true;
        // run callable synchronously
        auto t0 = MilliClock::now();
        try { c(); } catch (...) { /* swallow - cannot propagate */ }
        auto t1 = MilliClock::now();
        auto duration = std::chrono::duration_cast<ms>(t1 - t0).count();
        last_log_ = std::string("fake-thread finished in ") + std::to_string(duration) + " ms";
        joined_ = true;
    }

    // join is a no-op because the function already ran synchronously
    void join() {
        if (!started_) return;
        joined_ = true;
    }

    // retrieve last stored log (if any)
    std::string get_log() const { return last_log_; }

    // Non-copyable, movable
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&& other) noexcept {
        started_ = other.started_;
        joined_ = other.joined_;
        last_log_ = std::move(other.last_log_);
        other.started_ = other.joined_ = false;
    }
    Thread& operator=(Thread&& other) noexcept {
        if (this != &other) {
            started_ = other.started_;
            joined_ = other.joined_;
            last_log_ = std::move(other.last_log_);
            other.started_ = other.joined_ = false;
        }
        return *this;
    }

private:
    bool started_;
    bool joined_;
    std::string last_log_;
};

} // namespace MyThreadNoOS

#endif // MYTHREAD_NOOS_H
