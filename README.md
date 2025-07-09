# BugCacther-ITI

## Smart Agriculture Insect Monitoring System

This project includes the design of electronic circuits and 3D printed circuits to create an insect control system in an agricultural environment. Using embedded systems such as **ESP32** and cameras, insect photos will be sent to an AI model to estimate the health of a field.

## Features

### Hardware Components

- **ESP32 Microcontroller** - Main processing unit
- **DHT11 Sensor** - Temperature and humidity monitoring
- **I2C OLED Display (128x64)** - Real-time status visualization
- **Bluetooth Serial Communication** - Wireless data transmission
- **Physical Controls** - Measure button and display/bluetooth switches
- **LED Indicator** - System status feedback

### Software Capabilities

- **Real-time Environmental Monitoring** - Automated DHT sensor readings every 20 seconds
- **Bluetooth Control Interface** - Remote command execution via mobile devices
- **Interactive Display System** - Toggle-able OLED display with status information
- **Low Power Management** - Intelligent sleep mode when inactive
- **Robust Error Handling** - Automatic retry mechanisms for sensor failures

## Usage

Connect to device name: `BugCatcher-BT`

Available commands:

- `measure` - Get current temperature and humidity reading
- `wifipass` - Configure WiFi password (planned feature)
- `wificssid` - Configure WiFi SSID (planned feature)
