#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_SOFTAPNAME "esp32-config"
#define CONFIG_OTANAME "VuePower"

#define CONFIG_MAGIC 0xE1AAFF00

typedef struct
{
    uint32_t magic;

    char hostname[32];
    char wifi_ssid[32];
    char wifi_password[32];

    char mqtt_server[32];
    int mqtt_port;
    char mqtt_user[32];
    char mqtt_password[32];
    char mqtt_client[32];

    uint32_t verbose;
    uint32_t mqtt_publish;

    float frequency_calib;
    float sensor_calib_phase[3];
    float sensor_calib_channel[16];
    int sensor_phase[16];
} t_cfg;

extern t_cfg current_config;

#endif
