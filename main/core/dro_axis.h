#pragma once
#include <atomic>
#include <stdint.h>
#include <driver/pulse_cnt.h>

class DroAxis {
public:
    DroAxis();

    void init(const char* name, int pinA, int pinB, float step_um, int unitIndex);

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
    int unitIndex;
    pcnt_unit_handle_t pcntUnit = nullptr;
    pcnt_channel_handle_t channelA = nullptr;
    pcnt_channel_handle_t channelB = nullptr;
    mutable std::atomic<int64_t> accumulated;
    bool invertAxis;
    float globalReference_ = 0.0f;
    bool configured = false;

    void configureHardware();
    void releaseHardware();
    int64_t snapshotCounts();
};
