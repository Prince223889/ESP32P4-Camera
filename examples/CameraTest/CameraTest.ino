#include <ESP32P4Camera.h>

ESP32P4Camera camera;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32P4-Camera / single capture");

  if (!camera.begin()) {
    Serial.println("CAMERA_INIT=FAILED");
    return;
  }

  if (!camera.capture()) {
    Serial.println("CAMERA_CAPTURE=FAILED");
    return;
  }

  Serial.println("CAMERA_INIT=OK");
  Serial.println("CAMERA_CAPTURE=OK");
  camera.printInfo();
  Serial.println("One test frame is now held in PSRAM/RAM.");
  Serial.println("Use camera.writePPM(...) in your own sketch to export it.");
}

void loop() {
  delay(1000);
}
