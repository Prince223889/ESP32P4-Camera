#include "ESP32P4Camera.h"
#include "esp_heap_caps.h"

ESP32P4Camera::~ESP32P4Camera() {
  end();
  if (frameBuffer_) {
    heap_caps_free(frameBuffer_);
    frameBuffer_ = nullptr;
  }
}

bool ESP32P4Camera::begin() {
  Config config;
  return begin(config);
}

bool ESP32P4Camera::begin(const Config &config) {
  end();
  config_ = config;

#if !defined(CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE) || !CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE
  Serial.println("[ESP32P4-Camera] MIPI-CSI video device is disabled.");
  Serial.println("Enable CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE in the ESP-IDF configuration used by Arduino.");
  return false;
#else
  ESPVideoCamConfigClass camConfig;
  if (!camConfig.begin((i2c_port_num_t)config_.sccbI2CPort,
                       config_.sccbSclPin,
                       config_.sccbSdaPin,
                       config_.sccbFrequency)) {
    Serial.println("[ESP32P4-Camera] SCCB/I2C configuration failed.");
    return false;
  }

  ESPVideoCSIConfigClass csiConfig;
  if (!csiConfig.begin(camConfig, config_.dontInitCsiLdo)) {
    Serial.println("[ESP32P4-Camera] CSI configuration failed.");
    return false;
  }

  if (!video_.begin(csiConfig)) {
    Serial.println("[ESP32P4-Camera] Failed to initialize MIPI-CSI/ISP.");
    return false;
  }
  videoStarted_ = true;

  if (!captureDev_.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, config_.captureBufferCount)) {
    Serial.println("[ESP32P4-Camera] Failed to open /dev/video0.");
    end();
    return false;
  }

  if (!captureDev_.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
    Serial.println("[ESP32P4-Camera] RGB565 format was rejected by the video device.");
    end();
    return false;
  }

  if (!captureDev_.startCapture()) {
    Serial.println("[ESP32P4-Camera] Failed to start capture.");
    end();
    return false;
  }

  ready_ = true;
  return true;
#endif
}

void ESP32P4Camera::end() {
  ready_ = false;
  frameSize_ = 0;
  width_ = 0;
  height_ = 0;
  if (videoStarted_) {
    video_.end();
    videoStarted_ = false;
  }
}

bool ESP32P4Camera::ensureFrameCapacity(size_t bytes) {
  if (bytes <= frameCapacity_) return true;
  uint8_t *next = static_cast<uint8_t*>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!next) next = static_cast<uint8_t*>(heap_caps_malloc(bytes, MALLOC_CAP_8BIT));
  if (!next) return false;
  if (frameBuffer_) heap_caps_free(frameBuffer_);
  frameBuffer_ = next;
  frameCapacity_ = bytes;
  return true;
}

bool ESP32P4Camera::capture() {
  if (!ready_) return false;
  ESPVideoBufferClass buffer = captureDev_.captureBuffer();
  if (!buffer.valid()) return false;
  const size_t bytes = buffer.size();
  if (!ensureFrameCapacity(bytes)) return false;
  memcpy(frameBuffer_, buffer.data(), bytes);
  frameSize_ = bytes;
  width_ = buffer.getWidth();
  height_ = buffer.getHeight();
  return true;
}

bool ESP32P4Camera::capture(FrameInfo &info) {
  if (!capture()) return false;
  info.data = frameBuffer_;
  info.size = frameSize_;
  info.width = width_;
  info.height = height_;
  info.format = "RGB565";
  return true;
}

bool ESP32P4Camera::writePPM(Stream &out) const {
  if (!frameBuffer_ || !frameSize_ || !width_ || !height_) return false;
  out.printf("P6\n%lu %lu\n255\n", (unsigned long)width_, (unsigned long)height_);
  for (uint32_t y = 0; y < height_; ++y) {
    for (uint32_t x = 0; x < width_; ++x) {
      const size_t i = ((size_t)y * width_ + x) * 2u;
      uint16_t px = (uint16_t)frameBuffer_[i] | ((uint16_t)frameBuffer_[i + 1] << 8);
      uint8_t rgb[3] = {
        (uint8_t)(((px >> 11) & 0x1F) * 255 / 31),
        (uint8_t)(((px >> 5) & 0x3F) * 255 / 63),
        (uint8_t)((px & 0x1F) * 255 / 31)
      };
      out.write(rgb, sizeof(rgb));
    }
  }
  return true;
}

void ESP32P4Camera::printInfo(Stream &out) const {
  out.println("================================");
  out.println(" ESP32P4-Camera");
  out.println("================================");
  out.printf("Ready       : %s\n", ready_ ? "YES" : "NO");
  out.println("Interface   : MIPI-CSI");
  out.println("Pixel format: RGB565");
  out.printf("Width       : %lu\n", (unsigned long)width_);
  out.printf("Height      : %lu\n", (unsigned long)height_);
  out.printf("Frame bytes : %lu\n", (unsigned long)frameSize_);
  out.println("SCCB SDA    : GPIO7 (default)");
  out.println("SCCB SCL    : GPIO8 (default)");
  out.println("================================");
}
