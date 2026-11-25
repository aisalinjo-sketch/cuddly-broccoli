#pragma once

// Common kernel types and utilities
// This replaces Rust's core/std types for a freestanding kernel

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <new>
#include <limits>
#include <initializer_list>

// Fixed-width integer types (already in cstdint)
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u128 = __uint128_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using i128 = __int128_t;

using usize = size_t;
using isize = ptrdiff_t;

// Pointer types
using paddr_t = u64;  // Physical address
using vaddr_t = u64;  // Virtual address

// Kernel result type (like Rust's Result)
template<typename T, typename E = const char*>
class Result {
    union {
        T ok_value_;
        E err_value_;
    };
    bool is_ok_;

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

public:
    constexpr Result(T&& value) : ok_value_(std::move(value)), is_ok_(true) {}
    constexpr Result(const T& value) : ok_value_(value), is_ok_(true) {}
    constexpr Result(E&& error) : err_value_(std::move(error)), is_ok_(false) {}
    constexpr Result(const E& error) : err_value_(error), is_ok_(false) {}

    constexpr ~Result() {
        if (is_ok_) {
            ok_value_.~T();
        } else {
            err_value_.~E();
        }
    }

    constexpr bool is_ok() const { return is_ok_; }
    constexpr bool is_err() const { return !is_ok_; }

    constexpr T& unwrap() {
        if (!is_ok_) {
            panic("Result::unwrap() on Err value");
        }
        return ok_value_;
    }

    constexpr const T& unwrap() const {
        if (!is_ok_) {
            panic("Result::unwrap() on Err value");
        }
        return ok_value_;
    }

    constexpr E& unwrap_err() {
        if (is_ok_) {
            panic("Result::unwrap_err() on Ok value");
        }
        return err_value_;
    }

    constexpr const E& unwrap_err() const {
        if (is_ok_) {
            panic("Result::unwrap_err() on Ok value");
        }
        return err_value_;
    }

    template<typename F>
    constexpr auto map(F&& f) && -> Result<decltype(f(std::move(ok_value_))), E> {
        if (is_ok_) {
            return Result<decltype(f(std::move(ok_value_))), E>(f(std::move(ok_value_)));
        }
        return Result<decltype(f(std::move(ok_value_))), E>(std::move(err_value_));
    }

    template<typename F>
    constexpr auto and_then(F&& f) && -> decltype(f(std::move(ok_value_))) {
        if (is_ok_) {
            return f(std::move(ok_value_));
        }
        return decltype(f(std::move(ok_value_)))(std::move(err_value_));
    }
};

// Option type (like Rust's Option)
template<typename T>
class Option {
    union {
        T value_;
    };
    bool has_value_;

    Option(const Option&) = delete;
    Option& operator=(const Option&) = delete;

public:
    constexpr Option() : has_value_(false) {}
    constexpr Option(std::nullptr_t) : has_value_(false) {}
    constexpr Option(T&& value) : value_(std::move(value)), has_value_(true) {}
    constexpr Option(const T& value) : value_(value), has_value_(true) {}

    constexpr ~Option() {
        if (has_value_) {
            value_.~T();
        }
    }

    constexpr bool has_value() const { return has_value_; }
    constexpr explicit operator bool() const { return has_value_; }

    constexpr T& unwrap() {
        if (!has_value_) {
            panic("Option::unwrap() on None value");
        }
        return value_;
    }

    constexpr const T& unwrap() const {
        if (!has_value_) {
            panic("Option::unwrap() on None value");
        }
        return value_;
    }

    constexpr T unwrap_or(T default_value) && {
        if (has_value_) {
            return std::move(value_);
        }
        return default_value;
    }
};

// Panic function (to be implemented)
[[noreturn]] void panic(const char* message);

// Assert macro
#define KERNEL_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            panic(msg); \
        } \
    } while (0)

// Likely/unlikely hints
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Unreachable marker
#define unreachable() __builtin_unreachable()

// Offset of member in struct
#define offsetof_type(type, member) __builtin_offsetof(type, member)

// Container of macro (like Linux kernel's container_of)
#define container_of(ptr, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(ptr) - offsetof_type(type, member))

// Bit manipulation
constexpr u64 align_up(u64 addr, u64 align) {
    return (addr + align - 1) & ~(align - 1);
}

constexpr u64 align_down(u64 addr, u64 align) {
    return addr & ~(align - 1);
}

constexpr bool is_aligned(u64 addr, u64 align) {
    return (addr & (align - 1)) == 0;
}

// Page constants
constexpr u64 PAGE_SIZE = 4096;
constexpr u64 PAGE_MASK = ~(PAGE_SIZE - 1);

constexpr u64 PAGE_ALIGN_UP(u64 addr) { return align_up(addr, PAGE_SIZE); }
constexpr u64 PAGE_ALIGN_DOWN(u64 addr) { return align_down(addr, PAGE_SIZE); }

// String view (non-owning)
class StringView {
    const char* data_;
    usize size_;

public:
    constexpr StringView() : data_(nullptr), size_(0) {}
    constexpr StringView(const char* str) : data_(str), size_(str ? strlen(str) : 0) {}
    constexpr StringView(const char* str, usize len) : data_(str), size_(len) {}

    constexpr const char* data() const { return data_; }
    constexpr usize size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr char operator[](usize i) const { return data_[i]; }

    constexpr StringView substr(usize pos, usize len = npos) const {
        if (pos > size_) return StringView();
        if (len == npos || pos + len > size_) len = size_ - pos;
        return StringView(data_ + pos, len);
    }

    static constexpr usize npos = static_cast<usize>(-1);
};

// Simple spinlock
class SpinLock {
    volatile bool locked_ = false;

public:
    constexpr SpinLock() = default;

    void lock() {
        while (__atomic_test_and_set(&locked_, __ATOMIC_ACQUIRE)) {
            // Spin
            __builtin_ia32_pause();
        }
    }

    void unlock() {
        __atomic_clear(&locked_, __ATOMIC_RELEASE);
    }

    bool try_lock() {
        return !__atomic_test_and_set(&locked_, __ATOMIC_ACQUIRE);
    }
};

// RAII lock guard
template<typename Lock>
class LockGuard {
    Lock& lock_;
public:
    explicit LockGuard(Lock& lock) : lock_(lock) { lock_.lock(); }
    ~LockGuard() { lock_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

#define spinlock_guard(lock) LockGuard<decltype(lock)> guard(lock)

// Atomic operations
template<typename T>
class Atomic {
    T value_;
public:
    constexpr Atomic(T val = T{}) : value_(val) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const {
        return __atomic_load_n(&value_, order);
    }

    void store(T val, std::memory_order order = std::memory_order_seq_cst) {
        __atomic_store_n(&value_, val, order);
    }

    T fetch_add(T val, std::memory_order order = std::memory_order_seq_cst) {
        return __atomic_fetch_add(&value_, val, order);
    }

    T fetch_sub(T val, std::memory_order order = std::memory_order_seq_cst) {
        return __atomic_fetch_sub(&value_, val, order);
    }

    bool compare_exchange_weak(T& expected, T desired,
                                std::memory_order success,
                                std::memory_order failure) {
        return __atomic_compare_exchange_n(&value_, &expected, desired, false, success, failure);
    }

    T exchange(T val, std::memory_order order = std::memory_order_seq_cst) {
        return __atomic_exchange_n(&value_, val, order);
    }
};