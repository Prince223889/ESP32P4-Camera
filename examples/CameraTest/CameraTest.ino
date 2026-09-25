#include <ESP32P4Camera.h>

ESP32P4Camera camera;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-P4 CAMERA TEST");
  Serial.println("========================================");

  if (!camera.begin()) {
    Serial.println("[ERROR] Camera initialization failed.");
    Serial.println("Check the Arduino board options, PSRAM, the OV5647 connection, and I2C pins.");
    return;
  }

  Serial.println("[OK] Camera initialized.");

  ESP32P4Camera::FrameInfo frame;
  if (!camera.captureOnce(frame)) {
    Serial.println("[ERROR] Test capture failed.");
    return;
  }

  Serial.println("[OK] Test photo captured.");
  Serial.print("Width     : "); Serial.println(frame.width);
  Serial.print("Height    : "); Serial.println(frame.height);
  Serial.print("Format    : "); Serial.println(frame.formatName);
  Serial.print("Frame size: "); Serial.print((unsigned long)frame.size); Serial.println(" bytes");
  Serial.println("The frame buffer is released automatically after the test.");
}

void loop() {
  delay(1000);
}
