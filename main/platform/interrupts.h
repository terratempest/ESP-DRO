#pragma once
#include <cstdint>
#include "hardware_abstraction.h"
#include "dro_axis.h"

void setupAxisInterrupts(HardwareAbstraction& hw, DroAxis* axes, size_t num_axes);
