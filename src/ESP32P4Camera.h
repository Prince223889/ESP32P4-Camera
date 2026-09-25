#pragma once

#include <Arduino.h>

#if !defined(CONFIG_IDF_TARGET_ESP32P4)
#error "ESP32P4-Camera requires an ESP32-P4 target. In Arduino IDE select an ESP32P4 board."
#endif

#include <ESP_Video.h>

class ESP32P4Camera {
public:
  struct FrameInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    size_t size = 0;
    esp_video_format_t format = ESP_VIDEO_FORMAT_UNKNOWN;
    char formatName[24] = "UNKNOWN";
  };

  ESP32P4Camera();
  ~ESP32P4Camera();

  bool begin(uint8_t i2cPort = 0,
             int8_t sclPin = 8,
             int8_t sdaPin = 7,
             uint32_t i2cFrequency = 400000,
             size_t captureBuffers = 2);

  bool ready() const { return ready_; }
  void stop();

  // Captures one frame and returns metadata. The frame buffer is returned to
  // the ESP-Video buffer pool when this function returns.
  bool captureOnce(FrameInfo& outInfo);

  // Captures one RGB565 frame and writes it as a binary PPM (P6) stream.
  // This is useful when the destination is a file-like Print object.
  bool writePPM(Print& out, FrameInfo* outInfo = nullptr);

  // Captures one RGB565 frame and writes it as an uncompressed 24-bit BMP.
  // The BMP is written bottom-up as required by the BMP format.
  bool writeBMP(Print& out, FrameInfo* outInfo = nullptr);

  const FrameInfo& lastFrame() const { return lastFrame_; }
  void info(Print& out = Serial) const;

private:
  bool ensureStarted();
  static bool fillFrameInfo(const ESPVideoBufferClass& buffer, FrameInfo& outInfo);
  static void copyFormatName(const ESPVideoBufferClass& buffer, FrameInfo& outInfo);
  static void writeLE16(Print& out, uint16_t value);
  static void writeLE32(Print& out, uint32_t value);
  static uint32_t bmpRowStride(uint32_t width);
  static void rgb565ToRgb888(uint16_t p, uint8_t rgb[3]);

  ESPVideoClass video_;
  ESPVideoCaptureDevClass capture_;
  bool ready_ = false;
  FrameInfo lastFrame_{};
};

// Compatibility names used by early examples.
using P4Camera = ESP32P4Camera;
