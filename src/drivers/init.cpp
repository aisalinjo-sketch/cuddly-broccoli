// Device Drivers Initialization

#include "drivers.hpp"
#include "arch.hpp"

namespace drivers {

void init() {
    // Serial is already initialized in arch::init()
    // Other drivers would be initialized here
    arch::serial::write_str("Drivers initialized\n");
}

} // namespace drivers