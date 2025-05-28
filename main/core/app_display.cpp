#include "app_display.h"
#include "config.h"
#include "lvgl.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>
#include <esp_lcd_touch_gt911.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <stdlib.h>
#include <cstring>

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t tp = NULL;

// LVGL flush callback for the RGB panel
static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_display_flush_ready(disp);
}

void app_display_init() {
    // LVGL buffer allocation (in PSRAM if available)
    static lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    static lv_draw_buf_t draw_buf;
    lv_draw_buf_init(&draw_buf, LCD_H_RES, 40, LV_COLOR_FORMAT_NATIVE, LCD_H_RES, buf1, LCD_H_RES * 40 * sizeof(lv_color_t));
    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_draw_buffers(disp, &draw_buf, NULL);

    // Backlight GPIO (set to output, enable backlight)
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << LCD_PIN_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(LCD_PIN_BK_LIGHT, 1);

    // RGB panel config (all fields in order)
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 30,
            .hsync_back_porch = 16,
            .hsync_front_porch = 210,
            .vsync_pulse_width = 13,
            .vsync_back_porch = 10,
            .vsync_front_porch = 22,
            .flags = {
                .hsync_idle_low = false,
                .vsync_idle_low = false,
                .de_idle_high = false,
                .pclk_active_neg = true,
                .pclk_idle_high = false
            }
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 4,
        .dma_burst_size = 64,
        .hsync_gpio_num = LCD_PIN_HSYNC,
        .vsync_gpio_num = LCD_PIN_VSYNC,
        .de_gpio_num = LCD_PIN_DE,
        .pclk_gpio_num = LCD_PIN_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = DATA_GPIOS,
        .flags = {
            .disp_active_low = 0,
            .refresh_on_demand = 0,
            .fb_in_psram = 1,
            .double_fb = 0,
            .no_fb = 0,
            .bb_invalidate_cache = 0
        }
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    lv_display_set_flush_cb(disp, my_flush_cb);

    // I2C config for touch (in master mode, 400kHz)

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master = {.clk_speed = 400000},
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL
    };
    i2c_param_config(TOUCH_I2C_NUM, &i2c_conf);
    i2c_driver_install(TOUCH_I2C_NUM, I2C_MODE_MASTER, 0, 0, 0);

    // Touch panel config (all fields in order)
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_RST,
        .int_gpio_num = TOUCH_INT,
        .levels = {0, 0},
        .flags = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false
        },
        .process_coordinates = NULL,
        .interrupt_callback = NULL,
        .user_data = NULL,
        .driver_data = NULL
    };

    // Touch IO (I2C) config
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = TOUCH_I2C_ADDR,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16, 
        .lcd_param_bits = 0,
        .flags = {
            .dc_low_on_data = 0,
            .disable_control_phase = 1
        }
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(TOUCH_I2C_NUM, &io_config, &io_handle));
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &tp));

    // Register LVGL input driver for touch
    lv_indev_t* indev_touch = lv_indev_create();
    lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev_touch, disp);

    lv_indev_set_read_cb(indev_touch, [](lv_indev_t* indev, lv_indev_data_t* data) {
        extern esp_lcd_touch_handle_t tp; 
        uint16_t x = 0, y = 0;
        uint8_t touched = 0;
        
        esp_err_t err = esp_lcd_touch_read_data(tp);
        if (err != ESP_OK) {
            printf("[TOUCH] Error reading data: %d\n", err);
        }

        esp_lcd_touch_get_coordinates(tp, &x, &y, NULL, &touched, 1);

        if (touched) {
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PRESSED;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    });
}
