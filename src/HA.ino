
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Config.h>

#include <HA.h>
#include <Macros.h>

t_ha_info ha_info;
extern PubSubClient mqtt;

void ha_addstrarray(char *json_str, const char *name, const char *value, bool last = false)
{
    char tmp_buf[128];

    if (value && strlen(value) > 0)
    {
        int pos = 0;
        char values_buf[128];
        int out_pos = 0;

        values_buf[out_pos++] = '"';

        bool done = false;
        while (!done && out_pos < sizeof(values_buf))
        {
            switch (value[pos])
            {
            case ';':
                values_buf[out_pos++] = '"';
                if (value[pos + 1])
                {
                    values_buf[out_pos++] = ',';
                    values_buf[out_pos++] = '"';
                }
                break;

            case 0:
                values_buf[out_pos++] = '"';
                done = true;
                break;

            default:
                values_buf[out_pos++] = value[pos];
                break;
            }
            pos++;
        }
        values_buf[out_pos++] = '\000';

        snprintf(tmp_buf, sizeof(tmp_buf), "\"%s\": [%s]%c ", name, values_buf, (last ? ' ' : ','));
        strcat(json_str, tmp_buf);
    }
}

void ha_addstr(char *json_str, const char *name, const char *value, bool last = false)
{
    char tmp_buf[128];

    if (value && strlen(value) > 0)
    {
        snprintf(tmp_buf, sizeof(tmp_buf), "\"%s\": \"%s\"%c ", name, value, (last ? ' ' : ','));
        strcat(json_str, tmp_buf);
    }
}

void ha_addmqtt(char *json_str, const char *name, const char *value, t_ha_entity *entity, bool last = false)
{
    char tmp_buf[128];

    if (value && strlen(value) > 0)
    {
        char path_buffer[64];

        if (entity && entity->alt_name)
        {
            sprintf(path_buffer, value, entity->alt_name);
        }
        else
        {
            sprintf(path_buffer, value, current_config.mqtt_client);
        }
        snprintf(tmp_buf, sizeof(tmp_buf), "\"%s\": \"%s\"%c ", name, path_buffer, (last ? ' ' : ','));
        strcat(json_str, tmp_buf);
    }
}

void ha_addfloat(char *json_str, const char *name, float value, bool last = false)
{
    char tmp_buf[64];

    snprintf(tmp_buf, sizeof(tmp_buf), "\"%s\": \"%f\"%c ", name, value, (last ? ' ' : ','));
    strcat(json_str, tmp_buf);
}

void ha_addint(char *json_str, const char *name, int value, bool last = false)
{
    char tmp_buf[64];

    snprintf(tmp_buf, sizeof(tmp_buf), "\"%s\": \"%d\"%c ", name, value, (last ? ' ' : ','));
    strcat(json_str, tmp_buf);
}

void ha_publish()
{
    char *json_str = (char *)malloc(1024);
    char mqtt_path[128];
    char uniq_id[128];

    Serial.printf("[HA] Publish\n");

    sprintf(ha_info.cu, "http://%s/", WiFi.localIP().toString().c_str());

    for (int pos = 0; pos < ha_info.entitiy_count; pos++)
    {
        const char *type = NULL;

        switch (ha_info.entities[pos].type)
        {
        case ha_sensor:
            Serial.printf("[HA] sensor\n");
            type = "sensor";
            break;
        case ha_text:
            Serial.printf("[HA] text\n");
            type = "text";
            break;
        case ha_number:
            Serial.printf("[HA] number\n");
            type = "number";
            break;
        case ha_button:
            Serial.printf("[HA] button\n");
            type = "button";
            break;
        case ha_binary_sensor:
            Serial.printf("[HA] binary_sensor\n");
            type = "binary_sensor";
            break;
        case ha_select:
            Serial.printf("[HA] select\n");
            type = "select";
            break;
        case ha_light:
            Serial.printf("[HA] light\n");
            type = "light";
            break;
        default:
            Serial.printf("[HA] last one\n");
            break;
        }

        if (!type)
        {
            break;
        }

        sprintf(uniq_id, "%s_%s", ha_info.id, ha_info.entities[pos].id);

        Serial.printf("[HA]   uniq_id %s\n", uniq_id);
        sprintf(mqtt_path, "homeassistant/%s/%s/%s/config", type, ha_info.id, ha_info.entities[pos].id);

        Serial.printf("[HA]   mqtt_path %s\n", mqtt_path);

        strcpy(json_str, "{");
        ha_addstr(json_str, "name", ha_info.entities[pos].name);
        ha_addstr(json_str, "uniq_id", uniq_id);
        ha_addstr(json_str, "dev_cla", ha_info.entities[pos].dev_class);
        ha_addstr(json_str, "stat_cla", ha_info.entities[pos].state_class);
        ha_addstr(json_str, "ic", ha_info.entities[pos].ic);
        ha_addstr(json_str, "mode", ha_info.entities[pos].mode);
        ha_addstr(json_str, "ent_cat", ha_info.entities[pos].ent_cat);
        ha_addmqtt(json_str, "cmd_t", ha_info.entities[pos].cmd_t, &ha_info.entities[pos]);
        ha_addmqtt(json_str, "stat_t", ha_info.entities[pos].stat_t, &ha_info.entities[pos]);
        ha_addmqtt(json_str, "rgbw_cmd_t", ha_info.entities[pos].rgbw_t, &ha_info.entities[pos]);
        ha_addmqtt(json_str, "rgb_cmd_t", ha_info.entities[pos].rgb_t, &ha_info.entities[pos]);
        ha_addmqtt(json_str, "fx_cmd_t", ha_info.entities[pos].fx_cmd_t, &ha_info.entities[pos]);
        ha_addmqtt(json_str, "fx_stat_t", ha_info.entities[pos].fx_stat_t, &ha_info.entities[pos]);
        ha_addstrarray(json_str, "fx_list", ha_info.entities[pos].fx_list);
        ha_addmqtt(json_str, "val_tpl", ha_info.entities[pos].val_tpl, &ha_info.entities[pos]);
        ha_addstrarray(json_str, "options", ha_info.entities[pos].options);
        ha_addstr(json_str, "unit_of_meas", ha_info.entities[pos].unit_of_meas);

        switch (ha_info.entities[pos].type)
        {
        case ha_number:
            ha_addint(json_str, "min", ha_info.entities[pos].min);
            ha_addint(json_str, "max", ha_info.entities[pos].max);
            break;
        default:
            break;
        }

        strcat(json_str, "\"dev\": {");
        ha_addstr(json_str, "name", ha_info.name);
        ha_addstr(json_str, "ids", ha_info.id);
        ha_addstr(json_str, "cu", ha_info.cu);
        ha_addstr(json_str, "mf", ha_info.mf);
        ha_addstr(json_str, "mdl", ha_info.mdl);
        ha_addstr(json_str, "sw", ha_info.sw, true);
        strcat(json_str, "}}");

        Serial.printf("[HA]    topic '%s'\n", mqtt_path);
        Serial.printf("[HA]    content '%s'\n", json_str);

        if (!mqtt.publish(mqtt_path, json_str))
        {
            Serial.printf("[HA] publish failed\n");
        }
    }

    Serial.printf("[HA] done\n");
    free(json_str);
}

void ha_received(char *topic, const char *payload)
{
    for (int pos = 0; pos < ha_info.entitiy_count; pos++)
    {
        char item_topic[128];

        if (ha_info.entities[pos].cmd_t && ha_info.entities[pos].received)
        {
            sprintf(item_topic, ha_info.entities[pos].cmd_t, current_config.mqtt_client);
            if (!strcmp(topic, item_topic))
            {
                ha_info.entities[pos].received(&ha_info.entities[pos], ha_info.entities[pos].received_ctx, payload);

                if (ha_info.entities[pos].transmit)
                {
                    ha_info.entities[pos].transmit(&ha_info.entities[pos], ha_info.entities[pos].transmit_ctx);
                }
            }
        }

        if (ha_info.entities[pos].rgb_t && ha_info.entities[pos].rgb_received)
        {
            sprintf(item_topic, ha_info.entities[pos].rgb_t, current_config.mqtt_client);
            if (!strcmp(topic, item_topic))
            {
                ha_info.entities[pos].rgb_received(&ha_info.entities[pos], ha_info.entities[pos].rgb_received_ctx, payload);

                if (ha_info.entities[pos].transmit)
                {
                    ha_info.entities[pos].transmit(&ha_info.entities[pos], ha_info.entities[pos].transmit_ctx);
                }
            }
        }

        if (ha_info.entities[pos].fx_cmd_t && ha_info.entities[pos].fx_received)
        {
            sprintf(item_topic, ha_info.entities[pos].fx_cmd_t, current_config.mqtt_client);
            if (!strcmp(topic, item_topic))
            {
                ha_info.entities[pos].fx_received(&ha_info.entities[pos], ha_info.entities[pos].fx_received_ctx, payload);

                if (ha_info.entities[pos].transmit)
                {
                    ha_info.entities[pos].transmit(&ha_info.entities[pos], ha_info.entities[pos].transmit_ctx);
                }
            }
        }
    }
}

void ha_transmit(const t_ha_entity *entity, const char *value)
{
    if (!entity)
    {
        return;
    }

    if (!entity->stat_t)
    {
        return;
    }
    char item_topic[128];
    sprintf(item_topic, entity->stat_t, current_config.mqtt_client);

    if (!mqtt.publish(item_topic, value))
    {
        Serial.printf("[HA] publish failed\n");
    }
}

void ha_transmit_topic(const char *stat_t, const char *value)
{
    if (!stat_t)
    {
        return;
    }

    char item_topic[128];
    sprintf(item_topic, stat_t, current_config.mqtt_client);

    if (!mqtt.publish(item_topic, value))
    {
        Serial.printf("[HA] publish failed\n");
    }
}

void ha_transmit_all()
{
    for (int pos = 0; pos < ha_info.entitiy_count; pos++)
    {
        if (ha_info.entities[pos].transmit)
        {
            ha_info.entities[pos].transmit(&ha_info.entities[pos], ha_info.entities[pos].transmit_ctx);
        }
    }
}

void ha_setup()
{
    memset(&ha_info, 0x00, sizeof(ha_info));

    sprintf(ha_info.name, "%s", current_config.mqtt_client);
    sprintf(ha_info.id, "%06llX", ESP.getEfuseMac());
    sprintf(ha_info.cu, "http://%s/", WiFi.localIP().toString().c_str());
    sprintf(ha_info.mf, "g3gg0.de");
    sprintf(ha_info.mdl, "");
    sprintf(ha_info.sw, "v1." xstr(PIO_SRC_REVNUM) " (" xstr(PIO_SRC_REV) ")");
    ha_info.entitiy_count = 0;

    mqtt.setBufferSize(512);
}

void ha_connected()
{
    for (int pos = 0; pos < ha_info.entitiy_count; pos++)
    {
        char item_topic[128];
        if (ha_info.entities[pos].cmd_t && ha_info.entities[pos].received)
        {
            sprintf(item_topic, ha_info.entities[pos].cmd_t, current_config.mqtt_client);
            mqtt.subscribe(item_topic);
        }
        if (ha_info.entities[pos].rgb_t && ha_info.entities[pos].rgb_received)
        {
            sprintf(item_topic, ha_info.entities[pos].rgb_t, current_config.mqtt_client);
            mqtt.subscribe(item_topic);
        }
        if (ha_info.entities[pos].fx_cmd_t && ha_info.entities[pos].fx_received)
        {
            sprintf(item_topic, ha_info.entities[pos].fx_cmd_t, current_config.mqtt_client);
            mqtt.subscribe(item_topic);
        }
    }
    ha_publish();
    ha_transmit_all();
}

bool ha_loop()
{
    uint32_t time = millis();
    static uint32_t nextTime = 0;

    if (time >= nextTime)
    {
        ha_publish();
        ha_transmit_all();
        nextTime = time + 60000;
    }

    return false;
}

void ha_add(t_ha_entity *entity)
{
    if (!entity)
    {
        return;
    }

    if (ha_info.entitiy_count >= MAX_ENTITIES)
    {
        return;
    }
    memcpy(&ha_info.entities[ha_info.entitiy_count++], entity, sizeof(t_ha_entity));
}

int ha_parse_index(const char *options, const char *message)
{
    if (!options)
    {
        return -1;
    }

    int pos = 0;
    char tmp_buf[128];
    char *cur_elem = tmp_buf;

    strncpy(tmp_buf, options, sizeof(tmp_buf));

    while (true)
    {
        char *next_elem = strchr(cur_elem, ';');
        if (next_elem)
        {
            *next_elem = '\000';
        }
        if (!strcmp(cur_elem, message))
        {
            return pos;
        }

        if (!next_elem)
        {
            return -1;
        }

        cur_elem = next_elem + 1;
        pos++;
    }
}

void ha_get_index(const char *options, int index, char *text)
{
    if (!options || !text)
    {
        return;
    }

    int pos = 0;
    char tmp_buf[128];
    char *cur_elem = tmp_buf;

    strncpy(tmp_buf, options, sizeof(tmp_buf));

    while (true)
    {
        char *next_elem = strchr(cur_elem, ';');
        if (next_elem)
        {
            *next_elem = '\000';
        }
        if (pos == index)
        {
            strcpy(text, cur_elem);
            return;
        }

        if (!next_elem)
        {
            return;
        }

        cur_elem = next_elem + 1;
        pos++;
    }
}
