# Secure Industrial Loading Gate

**COMP50069 - Hardware, Microcontrollers and Sensors**

This project is a Secure Industrial Loading Gate developed using an ESP32. The system automatically detects a vehicle, opens the gate, monitors the vehicle and safely closes the gate after the vehicle leaves.

## Hardware Components
- ESP32 DevKit
- PIR Motion Sensor
- HC-SR04 Ultrasonic Sensor
- Potentiometer
- Servo Motor
- I2C OLED Display
- Buzzer
- Push Button

## Main Features
- PIR motion detection
- Ultrasonic vehicle confirmation at 15 cm
- Automatic gate opening and closing
- Vehicle presence monitoring
- Potentiometer adjustable hold-open time from 2 to 8 seconds
- Safety obstruction detection while the gate is closing
- Buzzer safety warning
- Push-button safety reset
- OLED status display
- Auto and Manual operating modes
- UART/Serial Monitor control and monitoring

## System Logic
1. The PIR sensor detects movement.
2. The ultrasonic sensor confirms that the vehicle is within 15 cm.
3. The servo opens the gate.
4. The ultrasonic sensor monitors whether the vehicle is still near the gate.
5. After the vehicle leaves, the potentiometer controls a 2-8 second delay.
6. The gate starts closing after the delay.
7. If an obstruction is detected within 20 cm while closing, the gate stops and the buzzer is activated.
8. After the obstruction is removed, the reset button allows the gate to reopen safely.

## Serial Commands
- `A` - Auto Mode
- `M` - Manual Mode
- `O` - Open Gate in Manual Mode
- `C` - Close Gate in Manual Mode

## Development
The system was developed and tested using Arduino IDE with an ESP32 microcontroller.
