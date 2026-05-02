#pragma once
#include <atomic>
#include <stdint.h>
#include <esp_attr.h>
#include <driver/pcnt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

class DroAxis {
public:
    DroAxis();

    void init(const char* name, int pinA, int pinB, float step_um, pcnt_unit_t unit);

    const char* name;
    int pinA, pinB;
    float step_um;
    volatile float position_um; // legacy compat, not used for tracking

    void begin();
    void zero();

    void  setStepUm(float step);
    float getStepUm() const;

    void setInvert(bool invert);
    bool getInvert() const;

    void  setPositionUm(float newPos);
    float getPositionUm();

    // Global Reference Position
    void  setGlobalReference(float ref) { globalReference_ = ref; }
    float getGlobalReference() const    { return globalReference_; }

    void simulateStep(bool forward);

private:
    pcnt_unit_t pcnt_unit;
    mutable std::atomic<int64_t> accumulated;
    bool invertAxis;
    float globalReference_ = 0.0f;
    bool configured = false;
    mutable portMUX_TYPE counterMux = portMUX_INITIALIZER_UNLOCKED;

    void configureHardware();
    int64_t snapshotCounts();
};
