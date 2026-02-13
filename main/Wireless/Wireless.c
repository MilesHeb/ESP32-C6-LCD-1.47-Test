#include "Wireless.h"

#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"

static const char *TAG = "wireless";

static EventGroupHandle_t s_ev;
static esp_netif_t *s_netif;
static const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_ev, WIFI_CONNECTED_BIT);
        // Keep retrying forever (ELM dongles can be flaky)
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_ev, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t apply_static_ip(const wireless_wifi_cfg_t *cfg)
{
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(s_netif));

    esp_netif_ip_info_t ip = {0};
    ip.ip.addr      = inet_addr(cfg->static_ip);
    ip.gw.addr      = inet_addr(cfg->gw);
    ip.netmask.addr = inet_addr(cfg->netmask);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(s_netif, &ip));
    return ESP_OK;
}

esp_err_t Wireless_WiFiConnect(const wireless_wifi_cfg_t *cfg)
{
    if (!cfg || !cfg->ssid) return ESP_ERR_INVALID_ARG;

    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Netif + Event loop (must only be called once globally)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_ev = xEventGroupCreate();
    s_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wicfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wicfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_cb, NULL));

    wifi_config_t wcfg = {0};
    strncpy((char*)wcfg.sta.ssid, cfg->ssid, sizeof(wcfg.sta.ssid));
    strncpy((char*)wcfg.sta.password, cfg->pass ? cfg->pass : "", sizeof(wcfg.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));

    if (cfg->use_static_ip) {
        ESP_ERROR_CHECK(apply_static_ip(cfg));
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to SSID='%s' ...", cfg->ssid);
    return ESP_OK;
}

bool Wireless_IsConnected(void)
{
    if (!s_ev) return false;
    return (xEventGroupGetBits(s_ev) & WIFI_CONNECTED_BIT) != 0;
}
