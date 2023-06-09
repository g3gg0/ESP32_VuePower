#pragma once

#define DIO_BUZZER 12
#define DIO_BUZZER_GND 27
#define DIO_SDA 21
#define DIO_SCL 22
#define DIO_LED 23

/* from https://github.com/emporia-vue-local/esphome/discussions/173 */
#define ATMEL_SWDIO 13
#define ATMEL_SWCLK 14
#define ATMEL_RST 26

/* hardware config
    0x4 - Emporia Vue 2 - "VUE Smart Home Energy Meter" - REV-4/REV-5
*/
#define HWCFG_0 34 0
#define HWCFG_1 35 0
#define HWCFG_2 32 1
#define HWCFG_3 33 0
