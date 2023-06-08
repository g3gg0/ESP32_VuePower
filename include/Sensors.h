#pragma once

typedef struct
{
    float current;
    float power_real;
    float power[3];
    float power_calc[3];
    float power_phase_match[3];
    int phase_match;
} sensor_ch_data_t;

typedef struct
{
    float frequency;
    float phase_voltage[3];
    float phase_power[3];
    float phase_current[3];
    float phase_angle[3];
    sensor_ch_data_t channels[16];

    float sensor_power_total[19];
    float sensor_power_daily[19];
} sensor_data_t;

extern sensor_data_t sensor_data;
