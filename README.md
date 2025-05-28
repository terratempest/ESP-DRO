# ESP-DRO

Dependencies: 

    lvgl/lvgl: ^9.2.2

    espressif/esp_lcd_touch_gt911: ^1.1.3


This firmware is for a Digital Read Out (DRO) system. It runs on an ESP32 and provides a touchscreen UI to display


Key Components & Structure

    main.cpp

        Entry point. Initializes hardware, LVGL graphics/UI, loads tool data, sets up axis interrupts, and runs the main loop to keep the UI updated.

    config.h

        Central hardware and feature configuration: pin assignments, scale resolution, display size, max tools, NVS (storage) parameters, and feature toggles.

    dro_axis.cpp

        Encapsulates logic for each linear axis. Handles GPIO setup, encoder signal decoding via interrupts, position tracking in microns, and zeroing.

    tool_manager.cpp

        Manages a list of tools with name and individual X/Z offsets. Provides add, remove, rename, and persistent save/load (via NVS) functions.

    preferences_wrapper.cpp

        Wraps ESP32 NVS (non-volatile storage) for easy read/write of strings, floats, and integers with schema version control.

    app_display.cpp

        Initializes the TFT display and capacitive touch panel. Sets up LVGL drawing buffers, backlight, RGB panel, and touch drivers.

    hardware_abstraction.cpp

        Provides a generic interface for attaching/detaching GPIO interrupts, and fetching system time in ms/us.

    interrupts.cpp

        Sets up and manages ISR services for the axes using the hardware abstraction layer.

    ui_manager.cpp

        Implements the main UI logic and event handling:

            Displays current axis positions, tool names, offsets.

            Allows zeroing, unit toggle, diameter/radius toggle, tool selection/editing/adding/removal.

            Handles popups for numeric input (for manual DRO value setting).

            Manages display sleep/wake behavior.

            Updates UI elements on every loop iteration.