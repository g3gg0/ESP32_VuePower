
./esptool.py  -b 921600 --port /dev/ttyS3 write_flash \
  0x1000 /mnt/c/Users/g3gg0/Documents/PlatformIO/Projects/ESP32_Vue/.pio/build/VuePower/bootloader.bin \
  0x8000 /mnt/c/Users/g3gg0/Documents/PlatformIO/Projects/ESP32_Vue/.pio/build/VuePower/partitions.bin \
  0xe000 /mnt/c/Users/g3gg0/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 /mnt/c/Users/g3gg0/Documents/PlatformIO/Projects/ESP32_Vue/.pio/build/VuePower/firmware.bin
