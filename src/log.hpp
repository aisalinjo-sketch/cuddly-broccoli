#pragma once

// Logging Facility

#include "common.hpp"
#include "arch.hpp"

namespace log {

enum class Level : u8 {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
};

// Current log level
static Level g_log_level = Level::Debug;

inline void set_level(Level level) {
    g_log_level = level;
}

inline const char* level_str(Level level) {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        default: return "UNKN ";
    }
}

template<typename... Args>
void log(Level level, const char* file, int line, const char* fmt, Args... args) {
    if (level < g_log_level) return;

    // Disable interrupts for atomic output
    arch::cpu::disable_interrupts();

    // Print timestamp (ticks)
    arch::serial::write_str("[");
    arch::serial::write_dec(arch::time::ticks());
    arch::serial::write_str("] ");

    // Print level
    arch::serial::write_str(level_str(level));
    arch::serial::write_str(" ");

    // Print file:line
    arch::serial::write_str(file);
    arch::serial::write_str(":");
    arch::serial::write_dec(line);
    arch::serial::write_str(" ");

    // Print formatted message
    char buffer[256];
    int len = __builtin_snprintf(buffer, sizeof(buffer), fmt, args...);
    arch::serial::write_str(buffer);
    arch::serial::write_str("\n");

    arch::cpu::enable_interrupts();
}

} // namespace log

// Logging macros
#define LOG_TRACE(fmt, ...) log::log(log::Level::Trace, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log::log(log::Level::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log::log(log::Level::Info,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log::log(log::Level::Warn,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log::log(log::Level::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)