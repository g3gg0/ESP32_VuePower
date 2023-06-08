
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <SPIFFS.h>

#include "HA.h"
#include "Config.h"
#include <esp_wifi.h>
#include <esp_task_wdt.h>

extern bool ota_active;
bool safemode = false;

const char *wakeup_reason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        return "external signal using RTC_IO";
    case ESP_SLEEP_WAKEUP_EXT1:
        return "external signal using RTC_CNTL";
    case ESP_SLEEP_WAKEUP_TIMER:
        return "timer";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        return "touchpad";
    case ESP_SLEEP_WAKEUP_ULP:
        return "ULP program";
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        return "undefined";
    default:
        return "unknown reason";
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

    Serial.printf("[i] Wakeup        '%s'\n", wakeup_reason());
    Serial.printf("[i] SDK:          '%s'\n", ESP.getSdkVersion());
    Serial.printf("[i] CPU Speed:    %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[i] Chip Id:      %06X\n", ESP.getEfuseMac());
    Serial.printf("[i] Flash Mode:   %08X\n", ESP.getFlashChipMode());
    Serial.printf("[i] Flash Size:   %08X\n", ESP.getFlashChipSize());
    Serial.printf("[i] Flash Speed:  %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("[i] Heap          %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf("[i] SPIRam        %d/%d\n", ESP.getFreePsram(), ESP.getPsramSize());
    Serial.printf("\n");
    Serial.printf("[i] Starting\n");

    led_setup();

    Serial.printf("[i]   Setup SPIFFS\n");
    if (!SPIFFS.begin(true))
    {
        Serial.println("[E]   SPIFFS Mount Failed");
    }

    cfg_read();
    safemode_startup();

    Serial.printf("[i]   Setup WiFi\n");
    wifi_setup();
    Serial.printf("[i]   Setup OTA\n");
    ota_setup();

    if (safemode)
    {
        Serial.printf("[E] ENTER SAFE MODE. No further initialization.\n");
        ota_enable();
        return;
    }

    Serial.printf("[i]   Setup Time\n");
    time_setup();
    Serial.printf("[i]   Setup Webserver\n");
    www_setup();
    Serial.printf("[i]   Setup MQTT\n");
    mqtt_setup();
    Serial.printf("[i]   Setup I/O\n");
    io_setup();
    Serial.printf("[i]   Setup Sensors\n");
    sensors_setup();
    Serial.printf("[i]   Setup Buzzer\n");
    buzz_setup();

    Serial.println("Setup done");

    buzz_beep(12000, 500);
}

void safemode_startup()
{
    current_config.boot_count++;
    cfg_save();
    Serial.printf("[i] Boot count: %d\n", current_config.boot_count);

    if (current_config.boot_count > CONFIG_MAXFAILS)
    {
        safemode = true;
    }
}

bool safemode_loop()
{
    if (current_config.boot_count)
    {
        if (millis() > 30000)
        {
            Serial.printf("[i] Successfully booted\n");
            current_config.boot_count = 0;
            cfg_save();
        }
    }
    return false;
}

void loop()
{
    bool hasWork = false;

    if (!ota_active)
    {
        hasWork |= wifi_loop();

        if (!safemode)
        {
            hasWork |= time_loop();
            hasWork |= mqtt_loop();
            hasWork |= www_loop();
            hasWork |= sensors_loop();
        }
    }
    hasWork |= ota_loop();
    safemode_loop();

    if (!hasWork)
    {
        delay(5);
    }
}
