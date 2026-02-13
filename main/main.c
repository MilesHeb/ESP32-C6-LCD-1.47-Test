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

void app_main(void)
{
    Wireless_Init();
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    SD_Init(); // SD must be initialized behind the LCD
    LCD_Init();
    BK_Light(50);
    LVGL_Init(); // returns the screen object

    /********************* Demo *********************/
    vTaskDelay(pdMS_TO_TICKS(150));
    ui_init();

    while (1)
    {
        lv_timer_handler();
        ui_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
