// Task Subsystem Initialization

#include "task.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace task {

void init() {
    scheduler::init();
}

} // namespace task