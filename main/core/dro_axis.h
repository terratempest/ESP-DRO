#pragma once
#include <atomic>
#include <stdint.h>

extern "C" void generic_axis_isr(void* arg);

class DroAxis {
public:
    DroAxis(); // Default constructor
    void init(const char* name, int pinA, int pinB, float step_um);

    const char* name;
    int pinA, pinB;
    float step_um;                 // Microns per pulse
    volatile float position_um;    // Position in microns (float for real calibration)
    
    void begin();
    void zero();

    // Calibration
    void  setStepUm(float step); // Set scale factor (microns per pulse)
    float getStepUm() const;

    // Invert axis direction
    void setInvert(bool invert);
    bool getInvert() const;

    // Position read/write (safe for ISR/UI)
    void  setPositionUm(float newPos);
    float getPositionUm() const;

    // Global Reference Position
    void  setGlobalReference(float ref) { globalReference_ =  ref; }
    float getGlobalReference() const    { return globalReference_; }
    
    // For simulation/testing
    void simulateStep(bool forward);

    // Interrupt handler (call from ISR)
    void handleInterrupt();

private:
    std::atomic<uint8_t> last_state;
    volatile int32_t tick_count = 0; 

    bool  invertAxis = false;
    float globalReference_ = 0.0f;
};
