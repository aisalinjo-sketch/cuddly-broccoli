// User Interface Implementation

#include "ui.hpp"
#include "arch.hpp"

namespace ui {

void init() {
    arch::serial::write_str("UI initialized\n");
}

void print(const char* str) {
    arch::serial::write_str(str);
}

void print_hex(u64 value) {
    arch::serial::write_hex(value);
}

void print_dec(u64 value) {
    arch::serial::write_dec(value);
}

void println(const char* str) {
    arch::serial::write_str(str);
    arch::serial::write_str("\n");
}

} // namespace ui