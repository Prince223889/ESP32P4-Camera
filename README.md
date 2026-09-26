# ESP32P4-Camera

Small Arduino wrapper around Espressif's `ESP_Video` MIPI-CSI API for ESP32-P4. It is intentionally thin: the Espressif video stack does the camera probing, ISP and capture; this library keeps the Arduino-facing API small and reusable.

## Target

Designed first for Waveshare ESP32-P4-WIFI6-DEV-KIT + OV5647. The board uses an ESP32-P4 with an ESP32-C6 wireless coprocessor and a 2-lane MIPI-CSI camera connector. The default camera-control bus used by Espressif's current P4 MIPI examples is SDA GPIO7 / SCL GPIO8. See the board documentation before changing wiring.

## Arduino settings

For the current Waveshare Arduino CI baseline:

- Core: Arduino-ESP32 3.3.11
- Board: ESP32P4 Dev Module
- PSRAM: enabled
- ChipVariant: postv3 for P4 v3.x boards supported by that core
- Flash: 16 MB
- Partition: `app3M_fat9M_16MB`

Legacy pre-v3 silicon requires a different profile. Do not mix binaries between revision families.

## Minimal sketch

```cpp
#include <ESP32P4Camera.h>
ESP32P4Camera camera;

void setup() {
  Serial.begin(115200);
  if (!camera.begin()) return;
  if (!camera.capture()) return;
  camera.printInfo();
}
void loop() {}
```

## Design notes

The library captures into a library-owned buffer, so the frame remains available after `capture()` returns. The underlying `ESPVideoBufferClass` is released immediately, which prevents exhausting the V4L2/MMAP capture buffers.

`writePPM(Stream&)` exports the last RGB565 frame as a portable PPM image. For browser delivery, use `ESP32P4-CameraWeb`, which converts the frame to BMP line-by-line without allocating a second full 24-bit image.

## Important limitation

A successful Arduino compile is not a hardware test. The MIPI connector, OV5647 module, board revision, PSRAM and camera-control wiring still have to be verified on the physical board.
