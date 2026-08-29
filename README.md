# Senior Safe Tech — IoT Fall Detection & Alert System

An embedded safety device that detects falls in real time and automatically alerts an emergency contact — built for people who cannot call for help themselves, including elderly individuals, patients, those with paralysis or limited mobility, and infants.

## Overview

The system continuously monitors body orientation using an accelerometer. When a sudden, fall-like change in orientation is detected, it sounds a local alarm and gives a short window to cancel a false alarm via a physical button. If not cancelled, it automatically sends an SOS SMS with a location link and follows up with a phone call to a designated emergency contact.

## Key Features

- **Real-time fall detection** using accelerometer-based orientation tracking
- **False-alarm cancellation window** — a button press stops the alert before it's sent
- **Automatic SOS alert** — SMS with location, followed by a phone call if unacknowledged
- **Self-diagnostic boot sequence** — checks every component on startup and reports specific fixes for any failure
- **Cooldown logic** to prevent duplicate alerts from a single fall event

## Hardware Components

- **ESP8266 NodeMCU** — Main microcontroller
- **ADXL345 Accelerometer** — Fall detection via orientation sensing (I2C)
- **SIM800C GSM Module** — Sends SMS alerts and places emergency calls
- **Active Buzzer** — Local audible alarm
- **Push Button** — Cancels a false-positive alert

## Wiring Summary

- **ADXL345** — I2C (SDA/SCL)
- **SIM800C** — SoftwareSerial, TX: D5, RX: D6 (requires independent 4.2V/2A power supply)
- **Buzzer** — D8
- **Button** — D7 (INPUT_PULLUP)

## Setup Instructions

1. Install the required libraries: `Adafruit_Sensor`, `Adafruit_ADXL345_U`
2. Insert a 2G-compatible SIM card (Airtel or Vi — Jio is not supported)
3. Replace the placeholder emergency contact number and GPS coordinates in the code with actual values
4. Flash the code to the NodeMCU and open the Serial Monitor at 9600 baud to view the diagnostic report

## Project Status

Fully functional prototype, built and tested independently. Planned improvements include live GPS integration and a companion mobile app for managing emergency contacts.

## Disclaimer

This is a working prototype developed for learning and demonstration purposes, not a certified medical device. The emergency contact number and location coordinates in this repository are placeholders and must be replaced before real-world use.
