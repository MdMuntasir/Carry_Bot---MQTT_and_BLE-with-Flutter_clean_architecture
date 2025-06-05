# Carry Bot

## Overview
Carry Bot is a robotic assistant designed to help elderly or ill individuals carry loads. It can be controlled manually via a Flutter-based mobile app or operate autonomously by following objects in front of it. The robot features weight measurement, alert notifications, and Bluetooth Low Energy (BLE) connectivity for seamless interaction with the app.

## Features
- **Automatic Object Following**: Uses ultrasonic sensors to detect and follow objects or people.
- **Manual Control**: Control the robot's movement through a Flutter mobile app.
- **Weight Monitoring**: Displays the current weight being carried using an Hx711 load cell module.
- **Alert Notifications**: Sends alerts to the user via the mobile app.

## Hardware Components
- **Microcontroller**: ESP32
- **Sensors**:
  - Ultrasonic Sensor (2x): Distance and depth measurement
  - IR Sensor (2x): Obstacle detection
  - Hx711: Weight measurement
- **Motors**:
  - DC Gear Motor (4x): Drives the robot
  - Servo Motor (SG90): Steering or load adjustment
- **Motor Drivers**: L293N (2x)
- **Wheels**: Plastic Wheel (4x), Caster Wheel
- **Chassis**: PVC sheet with PVC glue
- **Power**: 12V Battery
- **Connectors**: Jumper wires (Male-to-Female & Female-to-Female)

## Pin Connections
The ESP32 is connected to the components as follows:

- **Distance Ultrasonic Sensor**:
  - TRIG: Pin 33
  - ECHO: Pin 35
- **Depth Ultrasonic Sensor**:
  - TRIG: Pin 32
  - ECHO: Pin 34
- **IR Sensors**:
  - Left: Pin 14
  - Right: Pin 12
- **Servo Motor**: Pin 13
- **Hx711**:
  - DT: Pin 25
  - SCK: Pin 26
- **Motor Pins**:
  - Motor 1: Forward (23), Backward (22)
  - Motor 2: Forward (4), Backward (15)
  - Motor 3: Forward (21), Backward (19)
  - Motor 4: Forward (18), Backward (5)

## Technology Stack
- **Robot Firmware**: Arduino-based code for ESP32
- **Mobile App**: Flutter with BLE connectivity
- **Communication**: Bluetooth Low Energy (BLE)

## Repository Structure
- **carry_bot/**: Flutter app source code
  - `lib/`: Contains app logic, UI, and BLE communication
- **esp32/**: ESP32 firmware for controlling sensors, motors, and BLE communication

## Setup Instructions
### Prerequisites
- **Hardware**:
  - Assemble the robot using the listed components and pin connections.
  - Ensure the 12V battery is fully charged.
- **Software**:
  - Install the Arduino IDE for ESP32 programming.
  - Install Flutter and Dart for the mobile app development.
  - Install necessary dependencies for the Flutter app (see `carry_bot/pubspec.yaml`).

### Installation
1. **Robot Firmware**:
   - Clone the repository: `git clone https://github.com/MdMuntasir/Carry_Bot.git`
   - Open the `esp32` folder in the Arduino IDE.
   - Upload the code to the ESP32 using the specified pin connections.
2. **Mobile App**:
   - Navigate to the `carry_bot` folder.
   - Run `flutter pub get` to install dependencies.
   - Build and run the app on a mobile device using `flutter run`.
3. **Connection**:
   - Ensure the ESP32 is powered on.
   - Open the Carry Bot app and connect to the robot via BLE.

## Usage
1. **Automatic Mode**:
   - Power on the robot.
   - The robot will use ultrasonic sensors to follow objects in front of it.
2. **Manual Mode**:
   - Open the Carry Bot app.
   - Connect to the robot via BLE.
   - Use the app's interface to control the robot's movement.
3. **Weight Monitoring**:
   - The app displays the current weight measured by the Hx711 module.
4. **Alerts**:
   - The robot sends notifications to the app for events such as obstacles or low battery.

## Contributing
Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Commit your changes (`git commit -m "Add feature"`).
4. Push to the branch (`git push origin feature-branch`).
5. Create a pull request.

