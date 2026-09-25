#include "ESP32P4Camera.h"

ESP32P4Camera::ESP32P4Camera() = default;

ESP32P4Camera::~ESP32P4Camera() {
  stop();
}

bool ESP32P4Camera::begin(uint8_t i2cPort,
                          int8_t sclPin,
                          int8_t sdaPin,
                          uint32_t i2cFrequency,
                          size_t captureBuffers) {
  stop();

#if !defined(CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE) || !CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE
  (void)i2cPort;
  (void)sclPin;
  (void)sdaPin;
  (void)i2cFrequency;
  (void)captureBuffers;
  return false;
#else
  ESPVideoCamConfigClass camConfig;
  if (!camConfig.begin(static_cast<i2c_port_num_t>(i2cPort),
                       sclPin, sdaPin, i2cFrequency)) {
    return false;
  }

  ESPVideoCSIConfigClass csiConfig;
  if (!csiConfig.begin(camConfig)) {
    return false;
  }

  if (!video_.begin(csiConfig)) {
    return false;
  }

  if (!capture_.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, captureBuffers)) {
    video_.end();
    return false;
  }

  if (!capture_.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
    capture_.end();
    video_.end();
    return false;
  }

  if (!capture_.startCapture()) {
    capture_.end();
    video_.end();
    return false;
  }

  ready_ = true;
  return true;
#endif
}

bool ESP32P4Camera::ensureStarted() {
  return ready_ && capture_.isOpened() && capture_.isCaptureStarted();
}

void ESP32P4Camera::stop() {
  if (capture_.isCaptureStarted()) {
    capture_.stopCapture();
  }
  if (capture_.isOpened()) {
    capture_.end();
  }
  if (video_.isActive()) {
    video_.end();
  }
  ready_ = false;
}

void ESP32P4Camera::copyFormatName(const ESPVideoBufferClass& buffer,
                                   FrameInfo& outInfo) {
  const char* name = buffer.formatName();
  if (!name) {
    name = "UNKNOWN";
  }
  snprintf(outInfo.formatName, sizeof(outInfo.formatName), "%s", name);
}

bool ESP32P4Camera::fillFrameInfo(const ESPVideoBufferClass& buffer,
                                  FrameInfo& outInfo) {
  if (!buffer.valid()) {
    return false;
  }
  outInfo.width = buffer.getWidth();
  outInfo.height = buffer.getHeight();
  outInfo.size = buffer.size();
  outInfo.format = buffer.formatType();
  copyFormatName(buffer, outInfo);
  return outInfo.width != 0 && outInfo.height != 0;
}

bool ESP32P4Camera::captureOnce(FrameInfo& outInfo) {
  if (!ensureStarted()) {
    return false;
  }

  ESPVideoBufferClass buffer = capture_.captureBuffer();
  if (!fillFrameInfo(buffer, outInfo)) {
    return false;
  }
  lastFrame_ = outInfo;
  return true;
}

void ESP32P4Camera::writeLE16(Print& out, uint16_t value) {
  const uint8_t b[2] = {
      static_cast<uint8_t>(value & 0xFFu),
      static_cast<uint8_t>((value >> 8) & 0xFFu)};
  out.write(b, sizeof(b));
}

void ESP32P4Camera::writeLE32(Print& out, uint32_t value) {
  const uint8_t b[4] = {
      static_cast<uint8_t>(value & 0xFFu),
      static_cast<uint8_t>((value >> 8) & 0xFFu),
      static_cast<uint8_t>((value >> 16) & 0xFFu),
      static_cast<uint8_t>((value >> 24) & 0xFFu)};
  out.write(b, sizeof(b));
}

uint32_t ESP32P4Camera::bmpRowStride(uint32_t width) {
  const uint32_t raw = width * 3u;
  return (raw + 3u) & ~3u;
}

void ESP32P4Camera::rgb565ToRgb888(uint16_t p, uint8_t rgb[3]) {
  rgb[0] = static_cast<uint8_t>(((p >> 11) & 0x1Fu) * 255u / 31u);
  rgb[1] = static_cast<uint8_t>(((p >> 5) & 0x3Fu) * 255u / 63u);
  rgb[2] = static_cast<uint8_t>((p & 0x1Fu) * 255u / 31u);
}

bool ESP32P4Camera::writePPM(Print& out, FrameInfo* outInfo) {
  if (!ensureStarted()) {
    return false;
  }

  ESPVideoBufferClass buffer = capture_.captureBuffer();
  FrameInfo frame;
  if (!fillFrameInfo(buffer, frame) || frame.format != ESP_VIDEO_FORMAT_RGB565) {
    return false;
  }

  const size_t expected = static_cast<size_t>(frame.width) * frame.height * 2u;
  if (!buffer.data() || frame.size < expected) {
    return false;
  }

  out.print("P6\n");
  out.print(frame.width);
  out.print(' ');
  out.print(frame.height);
  out.print("\n255\n");

  const uint16_t* pixels = reinterpret_cast<const uint16_t*>(buffer.data());
  uint8_t rgb[3];
  for (uint32_t y = 0; y < frame.height; ++y) {
    const uint16_t* row = pixels + static_cast<size_t>(y) * frame.width;
    for (uint32_t x = 0; x < frame.width; ++x) {
      rgb565ToRgb888(row[x], rgb);
      if (out.write(rgb, sizeof(rgb)) != sizeof(rgb)) {
        return false;
      }
    }
  }

  lastFrame_ = frame;
  if (outInfo) {
    *outInfo = frame;
  }
  return true;
}

bool ESP32P4Camera::writeBMP(Print& out, FrameInfo* outInfo) {
  if (!ensureStarted()) {
    return false;
  }

  ESPVideoBufferClass buffer = capture_.captureBuffer();
  FrameInfo frame;
  if (!fillFrameInfo(buffer, frame) || frame.format != ESP_VIDEO_FORMAT_RGB565) {
    return false;
  }

  const size_t expected = static_cast<size_t>(frame.width) * frame.height * 2u;
  if (!buffer.data() || frame.size < expected) {
    return false;
  }

  const uint32_t rowStride = bmpRowStride(frame.width);
  const uint32_t imageSize = rowStride * frame.height;
  const uint32_t pixelOffset = 54u;
  const uint32_t fileSize = pixelOffset + imageSize;

  out.write(static_cast<uint8_t>('B'));
  out.write(static_cast<uint8_t>('M'));
  writeLE32(out, fileSize);
  writeLE16(out, 0);
  writeLE16(out, 0);
  writeLE32(out, pixelOffset);

  writeLE32(out, 40u);  // BITMAPINFOHEADER
  writeLE32(out, frame.width);
  writeLE32(out, frame.height); // positive = bottom-up
  writeLE16(out, 1u);
  writeLE16(out, 24u);
  writeLE32(out, 0u);   // BI_RGB
  writeLE32(out, imageSize);
  writeLE32(out, 2835u);
  writeLE32(out, 2835u);
  writeLE32(out, 0u);
  writeLE32(out, 0u);

  const uint16_t* pixels = reinterpret_cast<const uint16_t*>(buffer.data());
  const uint32_t padding = rowStride - frame.width * 3u;
  const uint8_t pad[3] = {0, 0, 0};
  uint8_t rgb[3];

  for (int32_t y = static_cast<int32_t>(frame.height) - 1; y >= 0; --y) {
    const uint16_t* row = pixels + static_cast<size_t>(y) * frame.width;
    for (uint32_t x = 0; x < frame.width; ++x) {
      rgb565ToRgb888(row[x], rgb);
      const uint8_t bgr[3] = {rgb[2], rgb[1], rgb[0]};
      if (out.write(bgr, sizeof(bgr)) != sizeof(bgr)) {
        return false;
      }
    }
    if (padding && out.write(pad, padding) != padding) {
      return false;
    }
  }

  lastFrame_ = frame;
  if (outInfo) {
    *outInfo = frame;
  }
  return true;
}

void ESP32P4Camera::info(Print& out) const {
  out.println("----------------------------------------");
  out.println("ESP32-P4 Camera");
  out.print("Ready: ");
  out.println(ready_ ? "YES" : "NO");
  out.print("Last width: ");
  out.println(static_cast<unsigned long>(lastFrame_.width));
  out.print("Last height: ");
  out.println(static_cast<unsigned long>(lastFrame_.height));
  out.print("Last bytes: ");
  out.println(static_cast<unsigned long>(lastFrame_.size));
  out.print("Last format: ");
  out.println(lastFrame_.formatName);
  out.println("----------------------------------------");
}
