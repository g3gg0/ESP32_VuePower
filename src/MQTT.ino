#define MQTT_DEBUG

#include <PubSubClient.h>
#include <ESP32httpUpdate.h>
#include <Config.h>
#include <Sensors.h>

#include "HA.h"

WiFiClient client;
PubSubClient mqtt(client);

extern int wifi_rssi;

uint32_t mqtt_last_publish_time = 0;
uint32_t mqtt_lastConnect = 0;
uint32_t mqtt_retries = 0;
bool mqtt_fail = false;

char command_topic[64];
char response_topic[64];

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print("'");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.print("'");
    Serial.println();

    payload[length] = 0;

    if (current_config.mqtt_publish & CONFIG_PUBLISH_HA)
    {
        ha_received(topic, (const char *)payload);
    }

    if (!strcmp(topic, command_topic))
    {
        char *command = (char *)payload;
        char buf[1024];

        if (!strncmp(command, "http", 4))
        {
            snprintf(buf, sizeof(buf) - 1, "updating from: '%s'", command);
            Serial.printf("%s\n", buf);

            mqtt.publish(response_topic, buf);
            ESPhttpUpdate.rebootOnUpdate(false);
            t_httpUpdate_return ret = ESPhttpUpdate.update(command);

            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;

            case HTTP_UPDATE_NO_UPDATES:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_NO_UPDATES");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;

            case HTTP_UPDATE_OK:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_OK");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                delay(500);
                ESP.restart();
                break;

            default:
                snprintf(buf, sizeof(buf) - 1, "update failed");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;
            }
        }
        else
        {
            snprintf(buf, sizeof(buf) - 1, "unknown command: '%s'", command);
            mqtt.publish(response_topic, buf);
            Serial.printf("%s\n", buf);
        }
    }
}

void mqtt_ota_received(const t_ha_entity *entity, void *ctx, const char *message)
{
    ota_setup();
}

void mqtt_setup()
{
    mqtt.setCallback(callback);

    if (current_config.mqtt_publish & CONFIG_PUBLISH_HA)
    {
        ha_setup();

        t_ha_entity entity;

        memset(&entity, 0x00, sizeof(entity));
        entity.id = "ota";
        entity.name = "Enable OTA";
        entity.type = ha_button;
        entity.cmd_t = "command/%s/ota";
        entity.received = &mqtt_ota_received;
        ha_add(&entity);

        memset(&entity, 0x00, sizeof(entity));
        entity.id = "rssi";
        entity.name = "WiFi RSSI";
        entity.type = ha_sensor;
        entity.stat_t = "feeds/integer/%s/rssi";
        entity.unit_of_meas = "dBm";
        ha_add(&entity);

        memset(&entity, 0x00, sizeof(entity));
        entity.id = "status";
        entity.name = "Status message";
        entity.type = ha_sensor;
        entity.stat_t = "live/%s/status";
        ha_add(&entity);

        memset(&entity, 0x00, sizeof(entity));
        entity.id = "frequency";
        entity.name = "Mains Frequency";
        entity.type = ha_sensor;
        entity.stat_t = "live/%s/frequency";
        entity.unit_of_meas = "Hz";
        entity.dev_class = "frequency";
        entity.state_class = "measurement";
        ha_add(&entity);

        for (int phase = 0; phase < 3; phase++)
        {
            char buf[32];

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_angle", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Angle", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/angle", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "°";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_voltage", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Voltage", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/voltage", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "V";
            entity.dev_class = "voltage";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_power", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Power", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/power", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "W";
            entity.dev_class = "power";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_power_total", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Power Total", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/power_total", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_power_draw_total", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Drawn", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/power_draw_total", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_power_inject_total", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Injected", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/power_inject_total", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_power_daily", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Power Daily", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/power_daily", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_current", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Current", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/current", phase + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "A";
            entity.dev_class = "current";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ph%d_status", phase + 1);
            entity.id = strdup(buf);
            sprintf(buf, "Phase #%d Status", phase + 1);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/phase_%d/status", phase + 1);
            entity.stat_t = strdup(buf);

            ha_add(&entity);
        }

        for (int channel = 0; channel < 16; channel++)
        {
            char buf[64];

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_power", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Power", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/power", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "W";
            entity.dev_class = "power";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_power_total", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Power Total", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/power_total", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_power_draw_total", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Drawn", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/power_draw_total", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_power_inject_total", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Injected", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/power_inject_total", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_power_daily", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Power Daily", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/power_daily", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "Wh";
            entity.dev_class = "energy";
            entity.state_class = "total_increasing";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_current", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Current", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/current", channel + 1);
            entity.stat_t = strdup(buf);
            entity.unit_of_meas = "A";
            entity.dev_class = "current";
            entity.state_class = "measurement";

            ha_add(&entity);

            memset(&entity, 0x00, sizeof(entity));
            sprintf(buf, "ch%d_status", channel + 1);
            entity.id = strdup(buf);
            sprintf(buf, "%s Status", current_config.channel_name[channel]);
            entity.name = strdup(buf);
            entity.type = ha_sensor;
            sprintf(buf, "live/%%s/ch%d/status", channel + 1);
            entity.stat_t = strdup(buf);

            ha_add(&entity);
        }
    }
}

void mqtt_publish_string(const char *name, const char *value)
{
    char path_buffer[128];

    sprintf(path_buffer, name, current_config.mqtt_client);

    if (!mqtt.publish(path_buffer, value))
    {
        mqtt_fail = true;
    }
    if (current_config.verbose & CONFIG_VERBOSE_SERIAL)
    {
        Serial.printf("Published %s : %s\n", path_buffer, value);
    }
}

void mqtt_publish_float(const char *name, float value)
{
    char path_buffer[128];
    char buffer[32];

    sprintf(path_buffer, name, current_config.mqtt_client);
    sprintf(buffer, "%0.4f", value);

    if (!mqtt.publish(path_buffer, buffer))
    {
        mqtt_fail = true;
    }
    if (current_config.verbose & CONFIG_VERBOSE_SERIAL)
    {
        Serial.printf("Published %s : %s\n", path_buffer, buffer);
    }
}

void mqtt_publish_int(const char *name, uint32_t value)
{
    char path_buffer[128];
    char buffer[32];

    if (value == 0x7FFFFFFF)
    {
        return;
    }
    sprintf(path_buffer, name, current_config.mqtt_client);
    sprintf(buffer, "%d", value);

    if (!mqtt.publish(path_buffer, buffer))
    {
        mqtt_fail = true;
    }
    if (current_config.verbose & CONFIG_VERBOSE_SERIAL)
    {
        Serial.printf("Published %s : %s\n", path_buffer, buffer);
    }
}

bool mqtt_loop()
{
    uint32_t time = millis();
    static uint32_t nextTime = 0;

#ifdef TESTMODE
    return false;
#endif
    if (mqtt_fail)
    {
        mqtt_fail = false;
        mqtt.disconnect();
    }

    MQTT_connect();

    if (!mqtt.connected())
    {
        return false;
    }

    mqtt.loop();

    if (current_config.mqtt_publish & CONFIG_PUBLISH_HA)
    {
        ha_loop();
    }

    if (time >= nextTime)
    {
        bool do_publish = false;
        nextTime = time + 5000;

        if ((time - mqtt_last_publish_time) > 5000)
        {
            do_publish = true;
        }

        if (do_publish)
        {
            char buf[64];
            Serial.printf("[MQTT] Publishing\n");

            /* debug */
            if (current_config.mqtt_publish & CONFIG_PUBLISH_DEBUG)
            {
                mqtt_publish_int("feeds/integer/%s/rssi", wifi_rssi);
            }

            /* publish */
            if (current_config.mqtt_publish & CONFIG_PUBLISH_MQTT)
            {
                mqtt_publish_string("live/%s/status", sensor_data.state);
                mqtt_publish_float("live/%s/frequency", sensor_data.frequency);

                for (int phase = 0; phase < 3; phase++)
                {
                    sensor_phase_data_t *ph = &sensor_data.phases[phase];

                    sprintf(buf, "live/%%s/phase_%d/status", phase + 1);
                    switch (ph->status)
                    {
                    case PHASE_STATUS_NOTCONNECTED:
                        mqtt_publish_string(buf, "Sensor not connected properly");
                        continue;

                    case PHASE_STATUS_OK:
                        mqtt_publish_string(buf, "OK");
                        break;
                    }
                    sprintf(buf, "live/%%s/phase_%d/voltage", phase + 1);
                    mqtt_publish_float(buf, ph->voltage);
                    sprintf(buf, "live/%%s/phase_%d/current", phase + 1);
                    mqtt_publish_float(buf, ph->current);
                    sprintf(buf, "live/%%s/phase_%d/angle", phase + 1);
                    mqtt_publish_float(buf, ph->angle);
                    sprintf(buf, "live/%%s/phase_%d/power", phase + 1);
                    mqtt_publish_float(buf, ph->power_filtered);
                    sprintf(buf, "live/%%s/phase_%d/power_total", phase + 1);
                    mqtt_publish_float(buf, ph->power_total);
                    sprintf(buf, "live/%%s/phase_%d/power_draw_total", phase + 1);
                    mqtt_publish_float(buf, ph->power_draw_total);
                    sprintf(buf, "live/%%s/phase_%d/power_inject_total", phase + 1);
                    mqtt_publish_float(buf, ph->power_inject_total);
                    sprintf(buf, "live/%%s/phase_%d/power_daily", phase + 1);
                    mqtt_publish_float(buf, ph->power_daily);
                    sprintf(buf, "live/%%s/phase_%d/status", phase + 1);
                }

                for (int ch = 0; ch < 16; ch++)
                {
                    sensor_ch_data_t *cur_ch = &sensor_data.channels[ch];

                    switch (cur_ch->status)
                    {
                    case CHANNEL_STATUS_NOTCONNECTED:
                        mqtt_publish_string(buf, "Sensor not connected properly");
                        continue;
                    case CHANNEL_STATUS_OK:
                        mqtt_publish_string(buf, "OK");
                        break;
                    }

                    for (int phase = 0; phase < 3; phase++)
                    {
                        sprintf(buf, "live/%%s/ch%d/rel_phase_%d/power", ch + 1, phase + 1);
                        mqtt_publish_float(buf, cur_ch->power[phase]);
                        sprintf(buf, "live/%%s/ch%d/rel_phase_%d/power_calc", ch + 1, phase + 1);
                        mqtt_publish_float(buf, cur_ch->power_calc[phase]);
                        sprintf(buf, "live/%%s/ch%d/rel_phase_%d/match", ch + 1, phase + 1);
                        mqtt_publish_float(buf, cur_ch->power_phase_match[phase]);
                    }
                    sprintf(buf, "live/%%s/ch%d/current", ch + 1);
                    mqtt_publish_float(buf, cur_ch->current);
                    sprintf(buf, "live/%%s/ch%d/power", ch + 1);
                    mqtt_publish_float(buf, cur_ch->power_filtered);
                    sprintf(buf, "live/%%s/ch%d/power_phase", ch + 1);
                    mqtt_publish_int(buf, cur_ch->phase_match);
                    sprintf(buf, "live/%%s/ch%d/power_total", ch + 1);
                    mqtt_publish_float(buf, cur_ch->power_total);
                    sprintf(buf, "live/%%s/ch%d/power_draw_total", ch + 1);
                    mqtt_publish_float(buf, cur_ch->power_draw_total);
                    sprintf(buf, "live/%%s/ch%d/power_inject_total", ch + 1);
                    mqtt_publish_float(buf, cur_ch->power_inject_total);
                    sprintf(buf, "live/%%s/ch%d/power_daily", ch + 1);
                    mqtt_publish_float(buf, cur_ch->power_daily);
                    sprintf(buf, "live/%%s/ch%d/status", ch + 1);
                }
            }
            mqtt_last_publish_time = time;
        }
    }

    return false;
}

void MQTT_connect()
{
    uint32_t curTime = millis();
    int8_t ret;

    if (strlen(current_config.mqtt_server) == 0)
    {
        return;
    }

    mqtt.setServer(current_config.mqtt_server, current_config.mqtt_port);

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if (mqtt.connected())
    {
        return;
    }

    if ((mqtt_lastConnect != 0) && (curTime - mqtt_lastConnect < (1000 << mqtt_retries)))
    {
        return;
    }

    mqtt_lastConnect = curTime;

    Serial.println("[MQTT] Connecting to MQTT... ");

    sprintf(command_topic, "tele/%s/command", current_config.mqtt_client);
    sprintf(response_topic, "tele/%s/response", current_config.mqtt_client);

    ret = mqtt.connect(current_config.mqtt_client, current_config.mqtt_user, current_config.mqtt_password);

    if (ret == 0)
    {
        mqtt_retries++;
        if (mqtt_retries > 8)
        {
            mqtt_retries = 8;
        }
        Serial.println("[MQTT] Retrying MQTT connection");
        mqtt.disconnect();
    }
    else
    {
        Serial.println("[MQTT] Connected!");
        mqtt.subscribe(command_topic);
        if (current_config.mqtt_publish & CONFIG_PUBLISH_HA)
        {
            ha_connected();
        }
    }
}
