
#include <Arduino.h>
#include <Wire.h>
#include <SparkFunLSM6DSO.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

LSM6DSO imu;
BLECharacteristic *pCharacteristic;

int stepCount = 0;
bool wasOverThreshold = false;

const float THRESHOLD = 1.2; // Puedes ajustar este valor
unsigned long lastStepTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA 21, SCL 22

  // Inicializar IMU
  if (imu.begin() != 0) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU initialized.");

  // Inicializar BLE
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

  Serial.println("BLE advertising started...");
}

void loop() {
  float accelY = imu.readFloatAccelY();
  unsigned long now = millis();

  // Detección de paso muy simple basada en umbral
  if (fabs(accelY) > THRESHOLD) {
    if (!wasOverThreshold && (now - lastStepTime > 300)) {
      stepCount++;
      lastStepTime = now;

      Serial.print("Step detected! Total: ");
      Serial.println(stepCount);

      String stepStr = String("Steps: ") + stepCount;
      pCharacteristic->setValue(stepStr.c_str());
      pCharacteristic->notify(); // Enviar vía BLE
    }
    wasOverThreshold = true;
  } else {
    wasOverThreshold = false;
  }

  delay(20); // 20ms = 50Hz
}
