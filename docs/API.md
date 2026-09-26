# API

## `ESP32P4Camera::begin()`

Starts the ESP32-P4 MIPI-CSI + ISP path and opens `/dev/video0` as RGB565.

## `capture()`

Dequeues one V4L2/MMAP frame, copies it into a library-owned buffer, then releases the driver buffer.

## `writePPM(Stream&)`

Exports the last captured RGB565 image as PPM (binary P6).
