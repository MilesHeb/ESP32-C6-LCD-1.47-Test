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

#include "OBD/elm327_tcp.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static const char *TAG = "elm_test";

// Forward declaration
static void elm_test_task(void *arg);

// --- Helpers: robust parsing (ignore junk, keep only hex) ---
static int extract_hex_only(const char *in, char *out, int out_sz)
{
    if (!in || !out || out_sz < 2) return 0;

    int j = 0;
    for (int i = 0; in[i] != 0 && j < out_sz - 1; i++) {
        char c = in[i];
        bool is_hex =
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f');

        if (is_hex) out[j++] = c;
    }
    out[j] = 0;
    return j;
}

static bool parse_pid_010c_rpm(const char *raw, float *rpm_out)
{
    if (!raw || !rpm_out) return false;

    char hex[256];
    extract_hex_only(raw, hex, sizeof(hex));

    // Look for "410C" (spaces already removed by extract_hex_only)
    const char *p = strstr(hex, "410C");
    if (!p) p = strstr(hex, "410c");
    if (!p) return false;

    // Need at least: 410C AABB => total 8 chars from '4'
    if ((int)strlen(p) < 8) return false;

    char a_str[3] = { p[4], p[5], 0 };
    char b_str[3] = { p[6], p[7], 0 };

    int A = (int)strtol(a_str, NULL, 16);
    int B = (int)strtol(b_str, NULL, 16);

    *rpm_out = ((A * 256) + B) / 4.0f;
    return true;
}

static void elm_test_task(void *arg)
{
    elm327_tcp_t elm = {
        .host = "192.168.0.10",
        .port = 0,
        .sock = -1
    };

    // Wait until WiFi is connected
    while (!Wireless_IsConnected()) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    ESP_LOGI(TAG, "WiFi connected. Starting port sweep on %s ...", elm.host);

    // Common ELM WiFi ports
    int ports[] = {23, 35000, 2000, 3000, 4000, 5000};
    bool connected = false;

    for (int i = 0; i < (int)(sizeof(ports) / sizeof(ports[0])); i++) {
        elm.port = ports[i];
        ESP_LOGI(TAG, "Trying port %d ...", elm.port);

        if (elm327_tcp_connect(&elm, 1200)) {
            ESP_LOGI(TAG, "Connected on port %d!", elm.port);
            connected = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (!connected) {
        ESP_LOGE(TAG, "No TCP ports responded");
        vTaskDelete(NULL);
        return;
    }

    // -------- ELM init sequence --------
    char rx[512];

    const char *init_cmds[] = {
        "ATZ",    // reset
        "ATE0",   // echo off
        "ATL0",   // linefeeds off
        "ATS0",   // spaces off
        "ATH0",   // headers off
        "ATSP0"   // auto protocol
    };

    for (int i = 0; i < (int)(sizeof(init_cmds) / sizeof(init_cmds[0])); i++) {
        ESP_LOGI(TAG, "Sending %s ...", init_cmds[i]);

        if (!elm327_tcp_send_cmd(&elm, init_cmds[i])) {
            ESP_LOGE(TAG, "Send failed on %s", init_cmds[i]);
            elm327_tcp_close(&elm);
            vTaskDelete(NULL);
            return;
        }

        int n = elm327_tcp_read_until_prompt(&elm, rx, sizeof(rx), 3000);
        if (n > 0) {
            ESP_LOGI(TAG, "Response to %s:\n%s", init_cmds[i], rx);
        } else {
            ESP_LOGW(TAG, "Timeout/no response on %s", init_cmds[i]);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // -------- RPM test (010C) --------
    ESP_LOGI(TAG, "Sending 010C (RPM) ...");

    if (!elm327_tcp_send_cmd(&elm, "010C")) {
        ESP_LOGE(TAG, "Send failed on 010C");
        elm327_tcp_close(&elm);
        vTaskDelete(NULL);
        return;
    }

    int n = elm327_tcp_read_until_prompt(&elm, rx, sizeof(rx), 4000);
    if (n > 0) {
        ESP_LOGI(TAG, "Raw RPM response:\n%s", rx);

        float rpm = 0.0f;
        if (parse_pid_010c_rpm(rx, &rpm)) {
            ESP_LOGI(TAG, "Parsed RPM: %.1f", rpm);
        } else {
            ESP_LOGW(TAG, "Failed to parse RPM from response");
        }
    } else {
        ESP_LOGW(TAG, "Timeout/no response on 010C");
    }

    // Keep socket alive for now
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    SD_Init();
    LCD_Init();
    BK_Light(50);

    LVGL_Init();

    vTaskDelay(pdMS_TO_TICKS(150));
    ui_init();

    wireless_wifi_cfg_t wifi_cfg = {
        .ssid = "WiFi_OBDII",
        .pass = "",
        .use_static_ip = true,
        .static_ip = "192.168.0.11",
        .gw = "192.168.0.10",
        .netmask = "255.255.255.0",
    };

    Wireless_WiFiConnect(&wifi_cfg);

    xTaskCreate(elm_test_task, "elm_test", 4096, NULL, 6, NULL);

    while (1)
    {
        lv_timer_handler();
        ui_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
