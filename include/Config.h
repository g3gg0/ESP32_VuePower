#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_SOFTAPNAME "esp32-config"
#define CONFIG_OTANAME "VuePower"
#define CONFIG_MAXFAILS 10

#define CONFIG_MAGIC 0xE1AAFF01

#define CONFIG_PUBLISH_MQTT 1
#define CONFIG_PUBLISH_HA 2
#define CONFIG_PUBLISH_DEBUG 4

#define CONFIG_VERBOSE_SERIAL 1

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

    uint16_t verbose;
    uint16_t boot_count;
    uint32_t mqtt_publish;

    float frequency_calib;
    float sensor_calib_phase[3];
    float sensor_calib_phase_voltage[3];
    float sensor_calib_channel[16];
    int sensor_phase[16];
    char channel_name[16][32];
} t_cfg;

extern t_cfg current_config;

#endif
