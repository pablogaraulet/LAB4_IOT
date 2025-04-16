#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// I2C address of the LSM6DSO (try 0x6B; if it does not respond, change to 0x6A)
#define LSM6DSO_ADDR 0x6B

// Sensor registers
#define CTRL1_XL   0x10  // Accelerometer configuration register
#define OUTX_L_A   0x28  // First X-axis acceleration register

// Function to write to a register
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Function to read 16 bits (two consecutive registers) with retry handling
int16_t read16bitRegister(uint8_t regL) {
  const int maxRetries = 3;
  for (int retry = 0; retry < maxRetries; retry++) {
    // Starts the transmission and writes the register address
    Wire.beginTransmission(LSM6DSO_ADDR);
    Wire.write(regL);
    uint8_t err = Wire.endTransmission(false); // Use of repeated start
    if(err != 0) {
      Serial.print("Error in endTransmission (attempt ");
      Serial.print(retry + 1);
      Serial.print("): ");
      Serial.println(err);
      delay(40);  // Waits 40 ms before retrying
      continue;
    }
    
    // Waits 40 ms for the sensor to prepare the response
    delay(40);
    
    // Requests 2 bytes from the sensor
    uint8_t numBytes = Wire.requestFrom((uint16_t)LSM6DSO_ADDR, (uint8_t)2);
    if (numBytes < 2) {
      Serial.print("Error: requestFrom returned only ");
      Serial.print(numBytes);
      Serial.print(" bytes (attempt ");
      Serial.print(retry + 1);
      Serial.println(")");
      delay(40);
      continue;
    }
    
    // Reads the 2 bytes and combines them into a 16-bit value
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();
    return (int16_t)((high << 8) | low);
  }
  // If it fails after the retries, returns 0
  return 0;
}

// --- BLE Configuration ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic = nullptr;

class MyBLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    // Processes data sent by the client if needed (not used in this example)
  }
};

// --- Variables for Step Counting ---
// During calibration, two thresholds are defined:
// THRESHOLD_G: The minimum acceleration magnitude to register a step (e.g., 1.3 g).
// RESET_THRESH_G: The magnitude below which the step is considered finished (e.g., 1.0 g).
static const float THRESHOLD_G = 1.3F;
static const float RESET_THRESH_G = 1.0F;

// The variable stepCount counts the number of detected steps.
volatile unsigned long stepCount = 0;
bool overThreshold = false;  // Flag to indicate that the magnitude has surpassed the threshold

// Sets up BLE and its characteristic
void setupBLE() {
  Serial.println("Starting BLE work!");
  BLEDevice::init("SDSUCS");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setCallbacks(new MyBLECallbacks());
  pCharacteristic->setValue("0");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE ready! Now you can read it on your phone!");
}

// Sets up the LSM6DSO sensor by directly accessing its registers
void setupIMU() {
  // Initializes I2C on SDA (GPIO21) and SCL (GPIO22) at 100 kHz
  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(100);

  Serial.println("🔧 Configuring LSM6DSO...");
  // Configures the accelerometer: 104 Hz and ±2g (CTRL1_XL = 0x40)
  writeRegister(CTRL1_XL, 0x40);
  delay(100);
  Serial.println("✅ Sensor configured. Reading acceleration...");
}

// Function to detect steps using the "raw" method
// It reads the acceleration registers, applies scaling, and calculates the acceleration vector magnitude.
// If the magnitude exceeds THRESHOLD_G and it is not already over threshold, a step is counted.
// When the magnitude falls below RESET_THRESH_G, the state is reset to allow a new count.
void detectSteps() {
  int16_t ax_raw = read16bitRegister(OUTX_L_A);
  int16_t ay_raw = read16bitRegister(OUTX_L_A + 2);
  int16_t az_raw = read16bitRegister(OUTX_L_A + 4);

  // For ±2g scale, the conversion factor is 0.000061 g/LSB
  float scale = 0.000061;
  float ax = ax_raw * scale;
  float ay = ay_raw * scale;
  float az = az_raw * scale;
  
  // Prints the acceleration values for reference on the Serial Monitor
  Serial.print("X: ");
  Serial.print(ax, 3);
  Serial.print(" g | Y: ");
  Serial.print(ay, 3);
  Serial.print(" g | Z: ");
  Serial.print(az, 3);
  Serial.println(" g");
  
  // Calculates the magnitude of the acceleration vector
  float mag = sqrt(ax * ax + ay * ay + az * az);

  // A step is detected if the magnitude exceeds the threshold and it has not been counted for that movement yet
  if (mag > THRESHOLD_G && !overThreshold) {
    stepCount++;
    overThreshold = true;
  } else if (mag < RESET_THRESH_G) {
    overThreshold = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("===== Starting BLE Step Counter with Raw Readings =====");

  setupIMU();   // Sets up the sensor via I2C
  setupBLE();   // Sets up BLE
}

void loop() {
  static unsigned long lastStepCount = 0;
  
  // Reads the sensor and processes step detection every 500 ms
  detectSteps();
  
  // If the step count has changed, the updated value is printed on the Serial Monitor
  if (stepCount != lastStepCount) {
    Serial.print("Steps = ");
    Serial.println(stepCount);
    lastStepCount = stepCount;
  }
  
  // Sends the step count via BLE
  char buff[16];
  snprintf(buff, sizeof(buff), "%lu", stepCount);
  pCharacteristic->setValue(buff);
  pCharacteristic->notify();
  
  delay(500);
}
