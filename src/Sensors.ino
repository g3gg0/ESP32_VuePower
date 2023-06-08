
// #include <Wire.h>
#include <Pins.h>
#include <Sensors.h>

#include "esp32-hal-i2c.h"

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
    uint16_t degrees[2];

    uint16_t current[19];

    uint16_t end;
};

sensor_data_t sensor_data;

SensorReading sensor_reading;

void sensors_setup()
{
    i2cInit(0, DIO_SDA, DIO_SCL, 800000);
}

bool sensors_loop()
{
    uint32_t time = millis();
    static uint32_t nextTime = 0;
    const uint32_t delta = 1000;
    const float hour_fract = 60 * 60 * 1000 / delta;

    if (time >= nextTime)
    {
        nextTime = time + delta;

        size_t rxLength;
        led_set(1, 1, 0, 0);
        int err = i2cRead(0, I2C_EMPORIA, (uint8_t *)&sensor_reading, sizeof(sensor_reading), 50, &rxLength);
        led_set(1, 0, 0, 0);

        if (err || rxLength != sizeof(sensor_reading))
        {
            // Serial.printf("I2C Read failure: %d, %d\n", err, rxLength);
            // mqtt_publish_string("test/%s/status", "I2C Read failure");
            return false;
        }

        if (sensor_reading.end != 0)
        {
            // mqtt_publish_string("test/%s/status", "Failed to read from sensor due to a malformed reading, should end in null bytes");
            return false;
        }

        if (!sensor_reading.is_unread)
        {
            // mqtt_publish_string("test/%s/status", "Ignoring sensor reading that is marked as read");
            return false;
        }

        if (sensor_reading.frequency == 0)
        {
            mqtt_publish_string("test/%s/status", "Ignoring sensor reading, frequency is zero");
            return false;
        }

        char buf[64];

        sensor_data.frequency = 25310.0f / (float)sensor_reading.frequency * current_config.frequency_calib;

        for (int phase = 0; phase < 3; phase++)
        {
            /* seems the power is negative when current is drawn from the grid, so negate*/
            sensor_data.phase_power[phase] = -sensor_reading.power[phase].phase[phase] * current_config.sensor_calib_phase[phase] / 5.5f;
            sensor_data.phase_voltage[phase] = sensor_reading.voltage[phase] * fabsf(current_config.sensor_calib_phase[phase]);
            sensor_data.phase_current[phase] = sensor_reading.current[phase] * 775.0 / 42624.0;
            sensor_data.phase_angle[phase] = (phase > 0) ? sensor_reading.degrees[phase - 1] * 360.0f / (float)sensor_reading.frequency : 0.0f;
        }

        for (int ch = 0; ch < 16; ch++)
        {
            sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];

            cur_ch->current = sensor_reading.current[3 + ch] * 775.0 / 170496.0;

            cur_ch->phase_match = 0;
            float phase_match_value = -1;

            for (int phase = 0; phase < 3; phase++)
            {
                cur_ch->power[phase] = sensor_reading.power[3 + ch].phase[phase] * current_config.sensor_calib_channel[ch] / 22.0f;
                cur_ch->power_calc[phase] = cur_ch->current * sensor_data.phase_voltage[phase];

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
        }

        /* statistics */
        for (int phase = 0; phase < 3; phase++)
        {
            sensor_data.sensor_power_total[phase] += sensor_data.phase_power[phase] / hour_fract;
            sensor_data.sensor_power_daily[phase] += sensor_data.phase_power[phase] / hour_fract;
        }

        for (int ch = 0; ch < 16; ch++)
        {
            sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];

            sensor_data.sensor_power_total[3 + ch] += cur_ch->power_real / hour_fract;
            sensor_data.sensor_power_daily[3 + ch] += cur_ch->power_real / hour_fract;
        }
    }
    return true;
}
