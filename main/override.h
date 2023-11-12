#pragma once

#include "freertos/FreeRTOSConfig.h"

// #undef configUSE_IDLE_HOOK
// #define configUSE_IDLE_HOOK             1

#undef configUSE_PREEMPTION
#define configUSE_PREEMPTION            0

#undef configUSE_TIME_SLICING
#define configUSE_TIME_SLICING          0

// #undef configIDLE_SHOULD_YIELD
// #define configIDLE_SHOULD_YIELD         0