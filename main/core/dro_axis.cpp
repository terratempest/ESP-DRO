#include "dro_axis.h"
#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_log.h>

// --- Interrupt handler: dispatch to the correct axis instance ---
extern "C" void IRAM_ATTR generic_axis_isr(void* arg) {
    if (arg) {
        static_cast<DroAxis*>(arg)->handleInterrupt();
    }
}

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
    // Configure pins as input with pull-up
    gpio_config_t io_conf   = {};
    io_conf.intr_type       = GPIO_INTR_ANYEDGE;
    io_conf.mode            = GPIO_MODE_INPUT;
    io_conf.pull_up_en      = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en    = GPIO_PULLDOWN_DISABLE;
    io_conf.pin_bit_mask    = (1ULL << pinA) | (1ULL << pinB);

    ESP_LOGI("DroAxis", "Configuring GPIOs: %d, %d", pinA, pinB);
    esp_err_t err = gpio_config(&io_conf);
    ESP_LOGI("DroAxis", "gpio_config ret = %d", err);

    // Read initial state
    uint8_t sA = gpio_get_level((gpio_num_t)pinA);
    uint8_t sB = gpio_get_level((gpio_num_t)pinB);
    last_state.store((sA << 1) | sB, std::memory_order_relaxed);

    // Attach interrupts (assume ISR service already installed by HAL)
    gpio_isr_handler_add((gpio_num_t)pinA, generic_axis_isr, this);
    gpio_isr_handler_add((gpio_num_t)pinB, generic_axis_isr, this);
}

void  DroAxis::zero() {position_um = 0.0f;}

void  DroAxis::setStepUm(float step) { step_um = step; }
float DroAxis::getStepUm() const { return step_um; }

void  DroAxis::setInvert(bool invert) { invertAxis = invert; }
bool  DroAxis::getInvert() const { return invertAxis; }

void  DroAxis::setPositionUm(float newPos) {position_um = newPos;}
float DroAxis::getPositionUm() const {return tick_count * step_um;}

void  DroAxis::simulateStep(bool forward) {position_um += forward ? step_um : -step_um;} //Debugging

void  DroAxis::handleInterrupt() {
    // Read both pins
    uint8_t sA = gpio_get_level((gpio_num_t)pinA);
    uint8_t sB = gpio_get_level((gpio_num_t)pinB);
    uint8_t state = (sA << 1) | sB;
    uint8_t prev = last_state.load(std::memory_order_relaxed);

    // Quadrature decode
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
