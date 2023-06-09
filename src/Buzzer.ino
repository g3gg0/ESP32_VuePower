
#include "Config.h"
#include <Pins.h>

#define BUZZER_LEDC 0

#define PWM_BITS 9
#define PWM_PCT(x) ((uint32_t)((100.0f - x) * ((1UL << (PWM_BITS)) - 1) / 100.0f))

void buzz_setup()
{
    pinMode(DIO_BUZZER, OUTPUT);
    pinMode(DIO_BUZZER_GND, OUTPUT);
    digitalWrite(DIO_BUZZER_GND, LOW);
}

bool buzz_loop()
{
    return false;
}

void buzz_on(uint32_t freq)
{
    ledcSetup(BUZZER_LEDC, freq, PWM_BITS);
    ledcAttachPin(DIO_BUZZER, BUZZER_LEDC);
    ledcWrite(BUZZER_LEDC, PWM_PCT(50));
}

void buzz_off()
{
    ledcWrite(BUZZER_LEDC, 0);
}

void buzz_beep(uint32_t freq, uint32_t duration)
{
    buzz_on(freq);
    delay(duration);
    buzz_off();
}
