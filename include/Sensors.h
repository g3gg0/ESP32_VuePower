#pragma once

#define CHANNEL_STATUS_OK 0
#define CHANNEL_STATUS_NOTCONNECTED 1

#define PHASE_STATUS_OK 0
#define PHASE_STATUS_NOTCONNECTED 1

typedef struct
{
    char name[32];
    float average;
    float per_second[60];
    uint32_t counter[60];
    uint32_t tm_min_last;
} sensor_minute_stat_t;

typedef struct
{
    float current;
    float power;
    float voltage;
    float angle;
    sensor_minute_stat_t minute_stats;
    float power_total;
    float power_draw_total;
    float power_inject_total;
    float power_daily;
    uint32_t status;
} sensor_phase_data_t;

typedef struct
{
    float current;
    float power_real;
    float power[3];
    float power_calc[3];
    float power_phase_match[3];
    sensor_minute_stat_t minute_stats;
    float power_total;
    float power_draw_total;
    float power_inject_total;
    float power_daily;
    int32_t phase_match;
    uint32_t status;
} sensor_ch_data_t;

typedef struct
{
    char state[64];
    float frequency;
    sensor_phase_data_t phases[3];
    sensor_ch_data_t channels[16];
} sensor_data_t;

extern sensor_data_t sensor_data;
