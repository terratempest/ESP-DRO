ESP-DRO

This is a digital readout (DRO) program for ESP32 powered TFT/Touch displays, designed for lathes (or mills) using TTL quadrature glass scales and a 7" Sunton touchscreen (800x480 with GT911 touch).
The coded in C++ and uses LVGL for the UI. Designed this mainly for my lathe where it's just handling 2 axes but code is modular and can be setup for additional axes provided you have enough free pins on your board to wire them up. 

![20250604_204626](https://github.com/user-attachments/assets/758a3c3e-7225-4b4a-93ea-753a7d6e396a)

What It Does:

* Reads multiple glass scale/TTL encoders in real-time (configurable axes)
* Shows all positions on a touchscreen DRO interface
* Stores global & per tool offsets
* Toggle between mm/inch, radius/diameter, incremental/absolute positioning
* All calibration, inversion, and tool data are persistent via NVS

Hardware Required

* ESP32-S3 powered LCD / Touch dev board (must have PSRAM) (RGB, GT911 touch)
* Linear scales with TTL quadrature output (5V)

Setup (Quick Start)
1. Install VSCode.
2. Install & configure ESP-IDF extension.
3. Clone Repository.
4. Open repository folder in VSCode.
4. Hardware Config
    
    Edit config.h to match your hardware:
    
        const AxisConfig AXES[] = {
            {"x", 17, 11, 5.0f}, // {name, pinA, pinB, resolution_um}
            {"z", 12, 13, 5.0f}, // add or remove axes as needed
        };
    
    Check all LCD/touch/encoder pins, adjust as needed for your board and wiring.
    Should work out of the box for the Sunton ESP32-S3 7" 800*480 TN Display with Touch

5. Build and Flash

Using the DRO

* All axes show up automatically.
* Tap the gear icon for calibration (set um/step and invert per axis).
* Use the tool dropdown to add, remove, or rename tools. Each tool stores unique offsets configurable with the TLO button per axis.
* Tap "IN" to switch between mm/inch, "DIA" for radius/diameter mode (only affects first axis).
* Tap ABS to toggle between incremental / absolute positioning. Incremental lets you use a temporary zero offset without effecting absolute position.
* All values are stored in flash and restored on reboot.

Customizing

* To add axes: Edit the AXES[] array in config.h and wire up more encoders.
* Change CONFIG_MAX_TOOLS if you need more tool slots.
* Everything else is in the UI—no code changes needed for common settings.

Structure

    main.cpp: Hardware setup and main loop

    config.h: All machine/display pin assignments and settings

    dro_axis.*: Handles encoder reading and axis state

    tool_manager.*: Tool storage and logic

    ui_manager.*, mainScreen.h: LVGL UI and event handling
