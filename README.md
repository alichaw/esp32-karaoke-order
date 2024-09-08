# Smart Augmented Vintage Jukebox Control System with ESP32
> Chinese Version sildes is [here](https://docs.google.com/presentation/d/1JKD7A5C1pfJTkAhMFqv6Ob-pRAtxeukd4btviUZhInc/edit?usp=sharing)

This project demonstrates the development of a smart control system for a vintage jukebox using an ESP32 microcontroller. The system allows users to remotely control and select songs through WiFi and QR code scanning, integrating modern technologies with a traditional jukebox interface.

## Project Description

The **Smart Augmented Vintage Jukebox Control System** combines modern technology with an old-fashioned jukebox to create a more user-friendly and efficient experience. The system is designed to cater to both younger users familiar with mobile technologies and older individuals accustomed to traditional jukeboxes. 

Key features include remote control via WiFi, song selection using QR codes, and integration with an infrared (IR) receiver to record signals from a remote. Additionally, it provides a web interface for song management and control.

### Features
- **Infrared Mode Capture**: Captures and records signals from a remote control using an IR receiver.
- **Song Selection via QR Code**: Generates a QR code for users to scan and choose songs from their mobile devices.
- **WiFi Remote Control**: Control the jukebox remotely over WiFi using a custom web interface.
- **Queue Management**: Tracks the current song queue and allows the modification of song order through an intelligent scheduling algorithm.
- **Compatibility with Older Jukebox Systems**: Designed to maintain compatibility with traditional jukebox interfaces while adding modern capabilities.

## Hardware Components
- **ESP32**: Core control unit that handles data processing and song control.
- **IR Receiver**: Records signals from the remote control.
- **TFT Display**: Displays the current status of the system and the song queue.
- **IR LED**: Sends song commands to the jukebox via infrared signals.

## Software Components
- **WiFi and Web Server**: The ESP32 hosts a web server that provides a control interface for users to manage song selection and the system.
- **Preferences Library**: Stores song queues and user preferences in the ESP32's internal flash memory.
- **QRCode API**: Generates QR codes for song selection and user identification.

## Demo Video
<img width="793" alt="截圖 2024-09-09 上午3 33 40" src="https://github.com/user-attachments/assets/a1dd6d35-0484-4d1a-b9b4-950e068f41ba">

Check out the system in action [here](https://youtu.be/069H8rDi6LI).

## Future Enhancements
- **Security Improvements**: Implement automatic customer ID generation and system shutdown features for added security.
- **Fair Song Ordering**: Introduce an advanced scheduling algorithm to ensure all users have an equal chance to play their songs.
- **Error Handling**: Add robust error handling to improve the system's stability and user experience during song submissions.

## Getting Started
### Prerequisites
- ESP32 development board
- IR receiver and LED
- TFT display
- Arduino IDE with ESP32 board package

### Installation
1. Clone this repository: `git clone https://github.com/alichaw/esp32-karaoke-order.git`
2. Open the project in Arduino IDE.
3. Flash the code to your ESP32 board.
4. Access the web interface via the ESP32's IP address on the local network.
