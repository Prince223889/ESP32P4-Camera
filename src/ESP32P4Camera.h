#pragma once
#include <Arduino.h>
#include <ESP_Video.h>

class ESP32P4Camera {
public:
  struct Config {
    int sccbI2CPort = 0;
    int8_t sccbSclPin = 8;
    int8_t sccbSdaPin = 7;
    uint32_t sccbFrequency = 400000;
    size_t captureBufferCount = 2;
    bool dontInitCsiLdo = false;
  };

  struct FrameInfo {
    const uint8_t *data = nullptr;
    size_t size = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    const char *format = "RGB565";
  };

  ESP32P4Camera() = default;
  ~ESP32P4Camera();

  ESP32P4Camera(const ESP32P4Camera&) = delete;
  ESP32P4Camera& operator=(const ESP32P4Camera&) = delete;

  bool begin();
  bool begin(const Config &config);
  void end();
  bool ready() const { return ready_; }

  bool capture();
  bool capture(FrameInfo &info);

  const uint8_t *data() const { return frameBuffer_; }
  size_t size() const { return frameSize_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

  bool writePPM(Stream &out) const;
  void printInfo(Stream &out = Serial) const;

private:
  bool ensureFrameCapacity(size_t bytes);

  Config config_{};
  ESPVideoClass video_{};
  ESPVideoCaptureDevClass captureDev_{};
  uint8_t *frameBuffer_ = nullptr;
  size_t frameCapacity_ = 0;
  size_t frameSize_ = 0;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  bool videoStarted_ = false;
  bool ready_ = false;
};
