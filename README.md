#  SmartCave: A Smart Parking System (ESP32 + Blynk)

A smart parking solution built with **ESP32** that monitors parking slot availability in real-time. It features an automated entry gate, local LCD status display, and cloud connectivity via the **Blynk IoT App**.

## Features

* **Real-time Slot Monitoring:** Uses 4 IR sensors to detect parking availability.
* **Automated Gate Control:** Opens automatically for entering cars (if slots are free) and exiting cars.
* **Smart Counting:** Tracks total cars entered/exited and calculates free slots.
* **Cloud Dashboard:** Syncs live data to the Blynk Mobile App/Web Dashboard.
* **Non-Blocking Logic:** The system continues to operate (sensors/gate) even if WiFi or Blynk connection is lost.
* **Visual Status:**
    * **LCD:** Pages between slot status (S1-S4) and general stats.
    * **LEDs:** Green for Gate Open, Red for Parking Full.

## 🛠️ Hardware Requirements

* **Microcontroller:** ESP32 Development Board
* **Sensors:**
    * 5x IR Obstacle Avoidance Sensors (4 for slots, 1 for exit)
    * 1x Ultrasonic Sensor (HC-SR04) for entry detection
* **Actuator:** 1x Servo Motor (SG90 or MG995)
* **Display:** 16x2 LCD with I2C Interface
* **Indicators:** Red & Green LEDs
* **Power Supply:** 5V External power (recommended for Servo)

## Pinout / Wiring

| Component | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **I2C LCD SDA** | GPIO 21 | Display Data |
| **I2C LCD SCL** | GPIO 22 | Display Clock |
| **Slot 1 IR** | GPIO 14 | Detects car in Slot 1 |
| **Slot 2 IR** | GPIO 27 | Detects car in Slot 2 |
| **Slot 3 IR** | GPIO 26 | Detects car in Slot 3 |
| **Slot 4 IR** | GPIO 25 | Detects car in Slot 4 |
| **Exit IR** | GPIO 35 | Detects exiting car |
| **Trig (Ultrasonic)**| GPIO 33 | Entry distance trigger |
| **Echo (Ultrasonic)**| GPIO 34 | Entry distance echo |
| **Servo Signal** | GPIO 13 | Gate Control |
| **Green LED** | GPIO 12 | Gate Open Indicator |
| **Red LED** | GPIO 4 | Parking Full Indicator |

## Libraries Required

Install these libraries via the Arduino IDE Library Manager:
1.  **Blynk** by Volodymyr Shymanskyy
2.  **ESP32Servo** by Kevin Harrington
3.  **LiquidCrystal I2C** by Frank de Brabander

## Blynk Configuration

Set up your Blynk Datastreams as follows:

| Virtual Pin | Data Type | Function |
| :--- | :--- | :--- |
| **V0** | Integer | Total Cars In |
| **V1** | Integer | Total Cars Out |
| **V2** | Integer | Free Slots Count |
| **V3** | String | Slot 1 Status |
| **V4** | String | Slot 2 Status |
| **V5** | String | Slot 3 Status |
| **V6** | String | Slot 4 Status |

## How to Run

1.  Clone this repository.
2.  Open `SmartParking.ino` in Arduino IDE.
3.  Update the **WiFi Credentials** and **Blynk Auth Token** at the top of the code:
    ```cpp
    #define BLYNK_AUTH_TOKEN "Your_Token_Here"
    char ssid[] = "Your_WiFi_SSID";
    char pass[] = "Your_WiFi_Pass";
    ```
4.  Select your ESP32 board and upload the code.
