#include <Arduino.h>
#include <Wire.h>
#include <SparkFunLSM6DSO.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

LSM6DSO imu;
BLECharacteristic *pCharacteristic;

int stepCount = 0;
bool wasOverThreshold = false;
float baseline = 0.0;

const float THRESHOLD = 0.3; // Threshold for RMS value (tune if needed)
unsigned long lastStepTime = 0;

void calibrateSensor() {
  Serial.println("Calibrating sensor... Keep the board still.");
  delay(1500);

  float sum = 0;
  for (int i = 0; i < 50; i++) {
    float x = imu.readFloatAccelX();
    float y = imu.readFloatAccelY();
    sum += sqrt((x * x + y * y) / 2.0);
    delay(20);
  }

  baseline = sum / 50.0;
  Serial.print("Calibration baseline (RMS): ");
  Serial.println(baseline, 4);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize IMU
  if (imu.begin() != 0) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU initialized.");

  calibrateSensor();

  // Initialize BLE
  BLEDevice::init("SDSUCS");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_READ
  );

  pCharacteristic->setValue("Step Counter Ready");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  Serial.println("BLE advertising started. Connect with your phone.");
}

void loop() {
  float x = imu.readFloatAccelX();
  float y = imu.readFloatAccelY();

  float rms = sqrt((x * x + y * y) / 2.0);
  float diff = fabs(rms - baseline);
  unsigned long now = millis();

  // Simple threshold step detection
  if (diff > THRESHOLD) {
    if (!wasOverThreshold && (now - lastStepTime > 300)) {
      stepCount++;
      lastStepTime = now;

      Serial.print("Step detected! Total steps: ");
      Serial.println(stepCount);

      String msg = String("Steps: ") + stepCount;
      pCharacteristic->setValue(msg.c_str());
      pCharacteristic->notify();
    }
    wasOverThreshold = true;
  } else {
    wasOverThreshold = false;
  }

  delay(20); // 50Hz
}
