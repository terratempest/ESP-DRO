#pragma once

//----------------------------------------
// MACHINE/PROJECT CONFIGURATION
//----------------------------------------

// Axis/Encoder config
typedef struct { 
    const char* name;
    int pinA;
    int pinB;
    float resolution_um; // axis-specific resolution in um/pulse
} AxisConfig;

// Config for linear scales
const AxisConfig AXES[] = {
    // {name, pinA, pinB, resolution}
    {"x", 17, 11, 5.0f}, // Axis 1
    {"z", 12, 13, 5.0f}, // Axis 2
    // Add more axis as required
};
#define AXES_COUNT (sizeof(AXES)/sizeof(AXES[0])) // Number of axes defined

// Number of tools supported (for array allocation, if needed)
#define CONFIG_MAX_TOOLS 16

// UI/TFT/LCD config (adjust for your display)
#define TFT_WIDTH   800
#define TFT_HEIGHT  480
#define LCD_H_RES TFT_WIDTH
#define LCD_V_RES TFT_HEIGHT

// Feature toggles (set to 1/0)
#define ENABLE_DIAMETER_MODE   1
#define ENABLE_METRIC_TOGGLE   1
#define ENABLE_TOOL_MANAGEMENT 1

//----------------------------------------
// PREFERENCES STORAGE
//----------------------------------------
#define NVS_NAMESPACE          "settings"
#define NVS_VERSION 1

//----------------------------------------
// LOGGING
//----------------------------------------
#define DEFAULT_LOG_LEVEL      LOG_INFO

//----------------------------------------
// DISPLAY & TOUCH
//----------------------------------------

// Pixel clock
#define LCD_PIXEL_CLOCK_HZ  12500000U

// Data lines
#define DATA_GPIOS      {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45}

// Board pin mapping
#define LCD_PIN_HSYNC    GPIO_NUM_39
#define LCD_PIN_VSYNC    GPIO_NUM_40
#define LCD_PIN_DE       GPIO_NUM_41
#define LCD_PIN_PCLK     GPIO_NUM_42
#define LCD_PIN_BK_LIGHT GPIO_NUM_2

#define TOUCH_RST        GPIO_NUM_38
#define TOUCH_INT        GPIO_NUM_18
#define TOUCH_I2C_SDA    GPIO_NUM_19
#define TOUCH_I2C_SCL    GPIO_NUM_20
#define TOUCH_I2C_NUM    I2C_NUM_0
#define TOUCH_I2C_ADDR   0x5D  // GT911 Address for your hardware