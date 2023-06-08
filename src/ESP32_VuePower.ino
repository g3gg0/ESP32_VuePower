
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <SPIFFS.h>

#include "HA.h"
#include <esp_wifi.h>
#include <esp_task_wdt.h>

extern bool ota_active;

void setup()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

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

    Serial.printf("[i]   Setup SPIFFS\n");
    if (!SPIFFS.begin(true))
    {
        Serial.println("[E]   SPIFFS Mount Failed");
    }
    cfg_read();
    led_setup();

    Serial.printf("[i]   Setup WiFi\n");
    wifi_setup();
    Serial.printf("[i]   Setup OTA\n");
    ota_setup();
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

void loop()
{
    bool hasWork = false;

    if (!ota_active)
    {
        hasWork |= wifi_loop();
        hasWork |= time_loop();
        hasWork |= mqtt_loop();
        hasWork |= www_loop();
        hasWork |= sensors_loop();
    }
    hasWork |= ota_loop();

    if (!hasWork)
    {
        delay(5);
    }
}
