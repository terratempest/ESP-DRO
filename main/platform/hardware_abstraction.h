#pragma once
#include <cstdint>
#include <functional>

// Type for interrupt callback; ctx is passed as void*
using InterruptCallback = std::function<void(void*)>;

class HardwareAbstraction {
public:
    HardwareAbstraction();

    // GPIO/Interrupts (see .cpp for ESP-IDF use)
    void attachInterrupt(int pin, InterruptCallback cb, void* context, int mode);
    void detachInterrupt(int pin);

    // Millisecond timer
    uint32_t millis() const;

    // Microsecond timer
    uint32_t micros() const;
};
