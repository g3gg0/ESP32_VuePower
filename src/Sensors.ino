

#include <Pins.h>
#include <Sensors.h>
#include <math.h>

#include "NTPTime.h"

#include "esp32-hal-i2c.h"

/* parts of it were taken from https://github.com/emporia-vue-local/esphome */

#define I2C_EMPORIA 0x64

struct __attribute__((__packed__)) ReadingPowerEntry
{
    int32_t phase[3];
};

struct __attribute__((__packed__)) SensorReading
{
    bool is_unread;
    uint8_t checksum;
    uint8_t unknown;
    uint8_t sequence_num;

    ReadingPowerEntry power[19];

    uint16_t voltage[3];
    uint16_t frequency;
    uint16_t angle[2];
    uint16_t current[19];

    uint16_t end;
};

sensor_data_t sensor_data;

SensorReading sensor_reading;

void sensors_setup()
{
    i2cInit(0, DIO_SDA, DIO_SCL, 800000);
    sprintf(sensor_data.state, "(init)");
}

void minute_stat_update(sensor_minute_stat_t *stat, float value)
{
    char msg[128];
    char buf[128];
    struct tm tm;
    getTime(&tm);

    if (stat->tm_min_last != tm.tm_min)
    {
        float sum = 0;
        int entries = 0;

        for (int pos = 0; pos < 60; pos++)
        {
            if (stat->counter[pos])
            {
                sum += stat->per_second[pos] / stat->counter[pos];
                entries++;
            }
            stat->per_second[pos] = 0;
            stat->counter[pos] = 0;
        }

        if (!entries)
        {
            entries = 1;
            sum = 0;
        }
        stat->average = sum / entries;
        stat->tm_min_last = tm.tm_min;
    }

    stat->per_second[tm.tm_sec] += value;
    stat->counter[tm.tm_sec]++;
}

bool sensors_loop()
{
    uint32_t time = millis();
    static int last_tm_yday = -1;
    static int last_tm_min = -1;
    static uint32_t nextTime = 0;
    const uint32_t delta = 500;
    struct tm tm;

    if (time >= nextTime)
    {
        uint32_t delta_real = time - nextTime;
        float hour_fract = (delta_real / 1000.0f) / 3600.0f;

        nextTime = time + delta;

        size_t rxLength;
        led_set(1, 1, 0, 0);
        int err = i2cRead(0, I2C_EMPORIA, (uint8_t *)&sensor_reading, sizeof(sensor_reading), 50, &rxLength);
        led_set(1, 0, 0, 0);

        if (err || rxLength != sizeof(sensor_reading))
        {
            sprintf(sensor_data.state, "I2C Read failure: %d, %d", err, rxLength);
            return false;
        }

        if (sensor_reading.end != 0)
        {
            sprintf(sensor_data.state, "I2C Read failure: incorrect magic");
            return false;
        }

        if (!sensor_reading.is_unread)
        {
            sprintf(sensor_data.state, "Ignoring sensor reading that is marked as read");
            return false;
        }

        if (sensor_reading.frequency == 0)
        {
            sprintf(sensor_data.state, "Ignoring sensor reading, frequency is zero");
            return false;
        }
        sprintf(sensor_data.state, "OK");

        sensor_data.frequency = 25310.0f / (float)sensor_reading.frequency * current_config.frequency_calib;

        for (int phase = 0; phase < 3; phase++)
        {
            sensor_phase_data_t *ph = &sensor_data.phases[phase];

            ph->power = sensor_reading.power[phase].phase[phase] * current_config.sensor_calib_phase[phase] / 5.5f;
            ph->voltage = sensor_reading.voltage[phase] * fabsf(current_config.sensor_calib_phase_voltage[phase]);
            ph->current = sensor_reading.current[phase] * 775.0 / 42624.0;
            ph->angle = (phase > 0) ? sensor_reading.angle[phase - 1] * 360.0f / (float)sensor_reading.frequency : 0.0f;

            /* when no sensor connected, the current is beyond <t.b.d> */
            if (ph->current > 150)
            {
                ph->status = PHASE_STATUS_NOTCONNECTED;
                ph->power = 0;
                ph->voltage = 0;
                ph->current = 0;
                ph->angle = 0;
                continue;
            }
            ph->power_filtered = ((POWER_PT1 - 1) * ph->power_filtered + ph->power) / POWER_PT1;
            ph->status = PHASE_STATUS_OK;
        }

        for (int ch = 0; ch < 16; ch++)
        {
            sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];

            cur_ch->current = sensor_reading.current[3 + ch] * 775.0 / 170496.0;

            /* when no sensor connected, the current is beyond 93A */
            if (cur_ch->current > 90)
            {
                cur_ch->status = CHANNEL_STATUS_NOTCONNECTED;
                cur_ch->power_real = 0;
                cur_ch->current = 0;
                continue;
            }
            cur_ch->status = CHANNEL_STATUS_OK;

            cur_ch->phase_match = 0;
            float phase_match_value = -1;

            for (int phase = 0; phase < 3; phase++)
            {
                cur_ch->power[phase] = sensor_reading.power[3 + ch].phase[phase] * current_config.sensor_calib_channel[ch] / 22.0f;

                /* ty to detect to which phase the current sensor is connected to */
                cur_ch->power_calc[phase] = cur_ch->current * sensor_data.phases[phase].voltage;

                if (fabsf(cur_ch->power_calc[phase]) > 0)
                {
                    const float pt1_value = 8;
                    float match = cur_ch->power[phase] / cur_ch->power_calc[phase];
                    cur_ch->power_phase_match[phase] = ((pt1_value - 1) * cur_ch->power_phase_match[phase] + match) / pt1_value;
                }
                else
                {
                    cur_ch->power_phase_match[phase] = 0;
                }

                if (cur_ch->power_phase_match[phase] > phase_match_value)
                {
                    phase_match_value = cur_ch->power_phase_match[phase];
                    cur_ch->phase_match = phase;
                }
            }

            /* autodetect phase or use overridden? */
            if (current_config.sensor_phase[ch] >= 0 && current_config.sensor_phase[ch] < 3)
            {
                cur_ch->phase_match = current_config.sensor_phase[ch];
            }
            cur_ch->power_real = cur_ch->power[cur_ch->phase_match];

            cur_ch->power_filtered = ((POWER_PT1 - 1) * cur_ch->power_filtered + cur_ch->power_real) / POWER_PT1;
        }

        /* statistics */
        getTime(&tm);

        if (tm.tm_yday != last_tm_yday)
        {
            /* reset statistics */
            for (int phase = 0; phase < 3; phase++)
            {
                sensor_data.phases[phase].power_daily = 0;
            }

            for (int ch = 0; ch < 16; ch++)
            {
                sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];

                cur_ch->power_daily = 0;
            }
            last_tm_yday = tm.tm_yday;
        }

        for (int phase = 0; phase < 3; phase++)
        {
            minute_stat_update(&sensor_data.phases[phase].minute_stats, sensor_data.phases[phase].power);
        }

        for (int ch = 0; ch < 16; ch++)
        {
            sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];
            minute_stat_update(&cur_ch->minute_stats, cur_ch->power_real);
        }

        /* summing up fractions of the power would end up in throwing away the value.
           adding 0.00xx to a number high as 100000 would have no effect.
           so average a minute and add every minute instead of multiple times a second.
           maybe later add multiple layers6 */
        if (tm.tm_min != last_tm_min)
        {
            last_tm_min = tm.tm_min;
            for (int phase = 0; phase < 3; phase++)
            {
                float energy = sensor_data.phases[phase].minute_stats.average / 60.0f;

                sensor_data.phases[phase].power_total += energy;
                sensor_data.phases[phase].power_daily += energy;

                if (energy >= 0)
                {
                    sensor_data.phases[phase].power_draw_total += energy;
                }
                else
                {
                    sensor_data.phases[phase].power_inject_total += -energy;
                }
            }

            for (int ch = 0; ch < 16; ch++)
            {
                sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];
                float energy = cur_ch->minute_stats.average / 60.0f;
                cur_ch->power_total += energy / 60.0f;
                cur_ch->power_daily += energy / 60.0f;

                if (energy >= 0)
                {
                    cur_ch->power_draw_total += energy;
                }
                else
                {
                    cur_ch->power_inject_total += -energy;
                }
            }
        }
    }
    return true;
}
