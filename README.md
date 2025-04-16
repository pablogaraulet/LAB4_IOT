Lab 4 – Bluetooth Step Counter
File Structure and How to Run the Code
This project is divided into two parts:

Part A is implemented in the file main.cpp.

Part B is implemented in the file src/step_counter.cpp.

We structured it this way because PlatformIO only compiles the file located inside the src folder. Therefore, if you want to compile and upload Part A, you need to move main.cpp into the src folder and move step_counter.cpp outside of it. Similarly, if you want to test Part B, make sure step_counter.cpp is inside src and main.cpp is outside or renamed.

Part A – Turn LED On/Off via Bluetooth
In this part, we used the ESP32's built-in BLE capability to create a simple BLE server. The server listens for commands sent from a phone using the nRF Connect app.

We implemented a BLE characteristic that receives a string message. If the message is "on" or "off", the ESP32 turns the onboard LED on or off accordingly. Any other message is ignored, and an error is printed in the serial monitor.

This allowed us to demonstrate basic BLE server functionality and real-time control of a hardware component (LED) from a mobile client.

DEMO VIDEO OF PART A: https://youtube.com/shorts/_Qm2EGarGkM?feature=share

CONEXIONS PART A:
![image](https://github.com/user-attachments/assets/d31aab6f-6d80-41b8-a548-2369e32bf6f2)
![image](https://github.com/user-attachments/assets/2980210f-9790-4f95-8448-59b1f34f3520)
![image](https://github.com/user-attachments/assets/8f2a1583-4446-46ff-8b2a-0bf78cbff722)


Part B – Step Counter with BLE Output
In this part, we connected the LSM6DSO accelerometer to the ESP32 via I2C and read acceleration values in real time. We calibrated the sensor and used a simple thresholding method to count steps:

If the total acceleration magnitude exceeds a certain threshold (1.3g), and was previously below it, we count a step.

We use another threshold (1.0g) to reset the state so we don't count the same movement multiple times.

Every time a step is detected, the total step count is updated and sent via BLE to the phone using the same nRF Connect app. The data can be seen live in the BLE client under the "SDSUCS" server name.

DEMO VIDEO FOR PART B: https://youtu.be/IyRbiv70N9s

CONEXIONS PART B:
