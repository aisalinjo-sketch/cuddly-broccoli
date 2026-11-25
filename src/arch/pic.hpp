#pragma once

// PIC (Programmable Interrupt Controller) Interface

#include "common.hpp"

namespace arch {
namespace pic {

void init();
void enable_irq(u8 irq);
void disable_irq(u8 irq);
void send_eoi(u8 irq);

} // namespace pic
} // namespace arch