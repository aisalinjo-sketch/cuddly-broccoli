#pragma once

// User Interface Interface

#include "common.hpp"

namespace ui {

// Initialize UI subsystem
void init();

// Print to console
void print(const char* str);
void print_hex(u64 value);
void print_dec(u64 value);
void println(const char* str);

} // namespace ui