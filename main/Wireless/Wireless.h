#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    const char *ssid;
    const char *pass;          // "" if open
    bool use_static_ip;        // optional but recommended for many ELM WiFi dongles
    const char *static_ip;     // e.g. "192.168.0.11"
    const char *gw;            // e.g. "192.168.0.10"
    const char *netmask;       // e.g. "255.255.255.0"
} wireless_wifi_cfg_t;

esp_err_t Wireless_WiFiConnect(const wireless_wifi_cfg_t *cfg);
bool Wireless_IsConnected(void);
