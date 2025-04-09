#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_PIN 2 

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.print("Received message: ");
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
      }
      Serial.println();

      // Convert to String and clean up the message
      String msg = String(value.c_str());
      msg.trim();         // Remove whitespace at start/end
      msg.toLowerCase();  // Convert to lowercase

      if (msg == "on") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED ON");
      } else if (msg == "off") {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED OFF");
      } else {
        Serial.println("Invalid command. Use 'on' or 'off'.");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with LED off

  BLEDevice::init("SDSUCS");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_READ
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Send 'on' or 'off'");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  Serial.println("BLE server ready. Send 'on' or 'off' from your phone.");
}

void loop() {
  delay(1000); 
}
