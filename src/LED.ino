

#include <Pins.h>

bool led_inhibit = false;

void led_setup()
{
    pinMode(DIO_LED, OUTPUT);
}

void led_set_adv(uint8_t n, uint8_t r, uint8_t g, uint8_t b, bool commit)
{
}

void led_set(uint8_t n, uint8_t r, uint8_t g, uint8_t b)
{
    if (n == 1)
    {
        digitalWrite(DIO_LED, !(r || g || b));
    }
}

void led_set_inhibit(bool state)
{
}

bool led_loop()
{
    return false;
}
