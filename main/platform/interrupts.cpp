#include "interrupts.h"
#include "esp_attr.h"
#include "driver/gpio.h"

// Generic ISR for all axes
void IRAM_ATTR genericAxisISR(void* ctx) {
    static_cast<DroAxis*>(ctx)->handleInterrupt();
}

bool isr_service_installed = false;

void setupAxisInterrupts(HardwareAbstraction& hw, DroAxis* axes, size_t num_axes) {
    // Enable GPIO ISR service if not already enabled
    if (!isr_service_installed) {
        gpio_install_isr_service(0);
        isr_service_installed = true;
    }

    // Loop through all axes, attach ISRs to A/B pins for each
    for (size_t i = 0; i < num_axes; ++i) {
        hw.attachInterrupt(axes[i].pinA, genericAxisISR, &axes[i], GPIO_INTR_ANYEDGE);
        hw.attachInterrupt(axes[i].pinB, genericAxisISR, &axes[i], GPIO_INTR_ANYEDGE);
    }
}
