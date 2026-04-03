# Fingerprint-Door-Lock
An Arduino-based biometric door lock using an Adafruit optical fingerprint sensor, LCD, and servo motor.
# 🔐 Fingerprint Door Lock System

An Arduino-based biometric door lock system. This project uses an optical fingerprint sensor to verify users, a 16x2 LCD for the user interface, and a servo motor to actuate the locking mechanism. It includes a physical button to easily toggle an "Enrollment Mode" to add new users on the fly.

## 🛠️ Hardware Requirements
* Arduino Nano
* Adafruit Optical Fingerprint Sensor (TTL Serial)
* JHD162A 16x2 LCD (4-bit parallel)
* SG90 / MG995 Servo Motor
* Push Button
* 10K Potentiometer (for LCD contrast)
* 220Ω Resistor (for LCD backlight)

## 📚 Required Libraries
Ensure you have the following libraries installed in your Arduino IDE:
* `LiquidCrystal.h` (Built-in)
* `Servo.h` (Built-in)
* `SoftwareSerial.h` (Built-in)
* `Adafruit_Fingerprint.h` (Install via Library Manager)

## ⚡ Wiring Guide

### LCD (JHD162A) → Arduino Nano
* **Pin 1 (VSS):** GND
* **Pin 2 (VDD):** 5V
* **Pin 3 (V0):** 10K pot wiper
* **Pin 4 (RS):** D12
* **Pin 5 (RW):** GND
* **Pin 6 (E):** D11
* **Pin 11 (D4):** D5
* **Pin 12 (D5):** D4
* **Pin 13 (D6):** D3
* **Pin 14 (D7):** D2
* **Pin 15 (A):** 5V (via 220Ω resistor)
* **Pin 16 (K):** GND

### Fingerprint Sensor
* **RED (VCC):** 3.3V or 5V
* **BLACK (GND):** GND
* **GREEN (TX):** D6 (Arduino RX)
* **WHITE (RX):** D7 (Arduino TX)

### Servo Motor
* **RED:** 5V *(External supply recommended for larger servos)*
* **BROWN:** GND *(Shared with Arduino GND)*
* **ORANGE:** D9 (Signal)

### Enrollment Button
* **Leg 1:** D8 *(Uses internal pull-up)*
* **Leg 2:** GND

## 🚀 How to Use
1. **Boot:** The system will initialize, lock the servo, and check for the sensor.
2. **Idle:** The screen will display "Place finger on sensor...".
3. **Unlock:** Place an enrolled finger on the scanner. The servo will open for 5 seconds before locking again.
4. **Enroll Mode:** Press the physical button to enter Enroll Mode. The system will guide you to place your finger twice to register a new user ID. Press the button again to exit.
