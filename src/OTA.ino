
#include <ArduinoOTA.h>

bool ota_active = false;
bool ota_setup_done = false;
uint32_t ota_offtime = 0;

void ota_setup()
{
    if (ota_setup_done)
    {
        ota_enable();
        return;
    }
    Serial.printf("[OTA] setHostname\n");
    ArduinoOTA.setHostname(CONFIG_OTANAME);

    Serial.printf("[OTA] onStart\n");
    ArduinoOTA.onStart([]()
                       {
        Serial.printf("[OTA] starting\n");
        led_set(0, 255, 0, 255);
        ota_active = true; 
        ota_offtime = millis() + 600000; })
        .onEnd([]()
               { ota_active = false; })
        .onProgress([](unsigned int progress, unsigned int total)
                    { led_set(0, 255 - (progress / (total / 255)), 0, (progress / (total / 255))); })
        .onError([](ota_error_t error)
                 {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

    Serial.printf("[OTA] begin\n");
    ArduinoOTA.begin();

    Serial.printf("[OTA] Setup finished\n");

    ota_setup_done = true;
    ota_enable();
}

void ota_enable()
{
    Serial.printf("[OTA] Enabled\n");
    ota_offtime = millis() + 600000;
}

bool ota_enabled()
{
    return (ota_offtime > millis() || ota_active);
}

bool ota_loop()
{
    if (ota_enabled())
    {
        ArduinoOTA.handle();
    }

    return ota_active;
}
