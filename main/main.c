/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "SD_SPI.h"
#include "RGB.h"
#include "Wireless.h"
#include "LVGL_Example.h"
#include "LVGL_UI/ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    SD_Init(); // SD must be initialized behind the LCD
    LCD_Init();
    BK_Light(50);

    LVGL_Init();

    vTaskDelay(pdMS_TO_TICKS(150));
    ui_init();

    // ---- WiFi connect to ELM327 adapter AP ----
    wireless_wifi_cfg_t wifi_cfg = {
        .ssid = "WiFi_OBDII",
        .pass = "",   // "" if open network
        .use_static_ip = true,     // try true first; if it won’t connect, set false
        .static_ip = "192.168.0.11",
        .gw = "192.168.0.10",
        .netmask = "255.255.255.0",
    };

    // Start WiFi (non-blocking)
    Wireless_WiFiConnect(&wifi_cfg);

    while (1)
    {
        // LVGL/UI loop
        lv_timer_handler();
        ui_update();

        // Optional: you can reflect wifi status in UI later
        // bool wifi_ok = Wireless_IsConnected();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
