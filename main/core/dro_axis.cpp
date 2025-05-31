#include "dro_axis.h"
#include <driver/gpio.h>
#include <esp_attr.h>

DroAxis::DroAxis()
    : name("init"), pinA(-1), pinB(-1), step_um(1.0f), position_um(0), last_state(0), invertAxis(false) {}

void DroAxis::init(const char* name, int pinA, int pinB, float step_um) {
    this->name = name;
    this->pinA = pinA;
    this->pinB = pinB;
    this->step_um = step_um;
    this->position_um = 0.0f;
    this->tick_count = 0;
    this->invertAxis = false;
}

void DroAxis::begin() {
    // Configure pins as input with pullup, interrupts disabled initially
    gpio_config_t io_conf = {};
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << pinA) | (1ULL << pinB);
    gpio_config(&io_conf);

    // Read initial state
    uint8_t sA = gpio_get_level((gpio_num_t)pinA);
    uint8_t sB = gpio_get_level((gpio_num_t)pinB);
    last_state.store((sA << 1) | sB, std::memory_order_relaxed);

    // Attach ISRs directly, passing 'this'
    gpio_isr_handler_add((gpio_num_t)pinA, generic_axis_isr, this);
    gpio_isr_handler_add((gpio_num_t)pinB, generic_axis_isr, this);

    // Now enable interrupts
    gpio_set_intr_type((gpio_num_t)pinA, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type((gpio_num_t)pinB, GPIO_INTR_ANYEDGE);
}

void DroAxis::zero() {
    position_um = 0.0f;
    tick_count = 0;
}

void DroAxis::setStepUm(float step) { step_um = step; }
float DroAxis::getStepUm() const { return step_um; }

void DroAxis::setInvert(bool invert) { invertAxis = invert; }
bool DroAxis::getInvert() const { return invertAxis; }

void DroAxis::setPositionUm(float newPos) {
    position_um = newPos;
    tick_count = static_cast<int32_t>(newPos / step_um);
}

float DroAxis::getPositionUm() const {
    return tick_count * step_um;
}

void DroAxis::simulateStep(bool forward) {
    tick_count += forward ? 1 : -1;
    position_um = tick_count * step_um;
}

void IRAM_ATTR DroAxis::handleInterrupt() {
    // Read both pins
    uint8_t sA = gpio_get_level((gpio_num_t)pinA) & 0x1;  // Only use LSB (0 or 1)
    uint8_t sB = gpio_get_level((gpio_num_t)pinB) & 0x1;
    uint8_t state = ((sA << 1) | sB) & 0x3;
    uint8_t prev = last_state.load(std::memory_order_relaxed) & 0x3;

    static const int8_t table[4][4] = {
        { 0,  -1,  1,  0 },
        { 1,   0,  0, -1 },
        {-1,   0,  0,  1 },
        { 0,   1, -1,  0 }
    };
    int8_t dir = table[prev][state];
    if (invertAxis) dir = -dir;
    if (dir != 0) {
        tick_count += dir;
    }
    last_state.store(state, std::memory_order_relaxed);
}

extern "C" void IRAM_ATTR generic_axis_isr(void* arg) {
    if (arg) static_cast<DroAxis*>(arg)->handleInterrupt();
}
