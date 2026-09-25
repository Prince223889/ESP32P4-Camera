# ESP32P4-Camera

A small Arduino library for the Waveshare ESP32-P4-WIFI6-DEV-KIT and similar ESP32-P4 boards with a MIPI-CSI camera.

## What it does

- Initializes the Espressif ESP-Video MIPI-CSI pipeline.
- Uses the board's shared camera I2C bus.
- Selects `RGB565` capture.
- Captures one test frame and reports its metadata.
- Can write the captured RGB565 frame as P6 PPM or 24-bit BMP to any Arduino `Print` destination.

## Default board wiring

For the Waveshare ESP32-P4-WIFI6-DEV-KIT with the OV5647 camera module, the default camera-control I2C pins used by this library are **SDA GPIO7** and **SCL GPIO8** at 400 kHz.

Do not move these pins to the ESP32-C6 Hosted-Wi-Fi SDIO pins. The board uses GPIO14-19 for that link.

## Basic example

```cpp
#include <ESP32P4Camera.h>
ESP32P4Camera camera;

void setup() {
  Serial.begin(115200);
  if (!camera.begin()) return;

  ESP32P4Camera::FrameInfo info;
  if (camera.captureOnce(info)) {
    Serial.printf("%lux%lu, %s, %lu bytes\\n",
                  (unsigned long)info.width,
                  (unsigned long)info.height,
                  info.formatName,
                  (unsigned long)info.size);
  }
}
void loop() {}
```

## Important Arduino IDE settings

Use the **ESP32-P4 Dev Module** (or the exact Waveshare-compatible P4 board entry available in your installed core), enable **PSRAM**, and use the board's post-v3 chip variant when available. The Waveshare Platform examples document `ChipVariant=postv3` for current boards and the OV5647 examples require PSRAM.

This library relies on the `ESP_Video` API supplied by a recent Arduino-ESP32 core. If the header or MIPI device is unavailable, update the core and check the board options before opening an issue.

## Notes about the photo

`captureOnce()` verifies that a real frame buffer was delivered and reports its dimensions/size. Raw pixels are not printed to Serial because an image is binary data. Use `writePPM()` or `writeBMP()` with a file/network `Print` destination when you need the actual image bytes.

## Compatibility aliases

The main class is `ESP32P4Camera`. `P4Camera` is kept as an alias for early examples.
