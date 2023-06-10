# ESP32_VuePower

See: https://www.g3gg0.de/wordpress/uncategorized/emporia-vue-2-custom-firmware/

Excerpt:

After a few hours, with the ESPHome fork as a great reference, I built a custom firmware for the Vue with Home Assistant support and bilancing counters to properly track how much power you get from the grid and how much you inject back. Tracking all the measurements per phase of course.

While I have still some problems understanding the exact way it measures current and the power in reference to the three phases, I still have some primitive autodetection, trying to find the phase which has the most correlation with the current signal. However the current I measure is sometimes far beyond what the power measurements show. This can not be only due to the power factor. I guess there is a current offset, that would have to be trained as well. But the current measurement isn’t really interesting, so I’d probably skip that.

For the case when the autodetection does not reliable detect the right phase – which is ithe case for e.g. solar inverters, driving energy into the grid – you can manually specify the correct phase (0, 1, 2) via the very simple web interface, so there is no doubt we are using the correct values. Init values are -1 for “autodetect”. In doubt, set manually, which channel is powered by which phase.
Screenshot of the web interface

Also the channels names can be configured in the web interface, so they show up in your Home Assistant with that name if you enabled that option. But make sure you do not use any special characters, as they might (actually will…) cause trouble in Home Assistant. Guess I should encode them, but… meh… some other day.

Flashing the firmware from PlatformIO did not work for me. I always got an non-explanatory error when esptool tried to flash the device.
So I ran it manually and that worked. Here is the command i used:

./esptool.py -b 921600 --port /dev/ttyS3 write_flash \
0x1000 ESP32_Vue/.pio/build/VuePower/bootloader.bin \
0x8000 ESP32_Vue/.pio/build/VuePower/partitions.bin \
0xe000 .platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
0x10000 ESP32_Vue/.pio/build/VuePower/firmware.bin

After flashing, the firmware will start an access point with the name esp32-config, waiting for you to connect, open up http://192.168.4.1 and configure WiFi. Then you can configure it from a proper web browser.
