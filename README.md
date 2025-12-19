# 🐾 IoT Pet Feeder (ESP32 + Web UI Simulation)

An **IoT-based Smart Pet Feeder** built and simulated using **ESP32**, **Wokwi**, and a **Web Dashboard**.  
The system supports **scheduled feeding**, **manual feeding**, **real-time weight monitoring**, and **remote control via a Web UI**, all without physical hardware.

---

## 📌 Project Overview

This project simulates an automated pet feeding system that:
- Dispenses food based on **scheduled times**
- Supports **manual feeding via Web UI**
- Monitors bowl weight using a **simulated load cell (HX711)**
- Uses an **RTC module** for time-based scheduling
- Exposes a **REST-style HTTP API**
- Provides a **modern Web Dashboard** hosted directly on the ESP32

The entire system is implemented and tested in **Wokwi (software-only simulation)**.

---

## 🎯 Key Features

- 🕒 Scheduled Feeding (multiple time slots)
- 🖐️ Manual Feeding (additive feeding logic)
- ⚖️ Live Bowl Weight Monitoring
- 🌐 Web-based Dashboard (HTML / CSS / JavaScript)
- 🔁 System Reset via Web UI
- 📜 Feeding History Log
- 🔌 REST API for external clients
- 🧪 No physical hardware required

---

## 🧱 System Architecture
[ Web Browser ]
|
| HTTP (JSON)
v
[ ESP32 Web Server ]
|
|-- Feeding Logic
|-- Schedule Logic
|-- Servo Control
|-- HX711 (Simulated)
|-- RTC


- The **ESP32** runs both the firmware and the HTTP server
- The **Web UI** communicates using REST APIs
- Firmware logic and UI are cleanly separated

---

## 🔌 API Endpoints

| Endpoint | Method | Description |
|--------|--------|------------|
| `/api/status` | GET | Get system status (weight, feeding state, schedule) |
| `/api/manual-feed?amount=X` | POST | Trigger manual feeding |
| `/api/set-slot` | POST | Configure feeding schedule slot |
| `/api/reset` | POST | Reset system state |

All responses are returned in **JSON format**.

---

## 🖥️ Web Dashboard Features

- Live bowl weight display
- Feeding status indicator
- Schedule configuration (time & weight)
- Manual feeding control
- System reset button
- Feeding history log

The UI is served directly from the ESP32 using **SPIFFS / PROGMEM**.

---

## 🛠️ Tech Stack

### Firmware
- ESP32 (Arduino Framework)
- PlatformIO
- HX711 (Load Cell – simulated)
- RTC (time scheduling)
- Servo motor control
- SPIFFS / PROGMEM

### Web
- HTML
- CSS
- JavaScript (Fetch API)
- REST architecture

### Simulation
- Wokwi
- VS Code
- PlatformIO

---

## 📂 Project Structure
├── diagram.json # Wokwi circuit diagram
├── platformio.ini
├── partitions.csv
├── wokwi.toml
├── src/
│ └── main.cpp # Firmware + API logic
├── include/
│ └── index_html.h # Web UI (HTML / CSS / JS)
└── README.md


---

## 📈 Benefits of the Project

- No physical hardware required
- Easy to demonstrate and debug
- Modular and extendable design
- Real-world IoT architecture
- Suitable for academic demos and portfolios

---

## 📷 Demo & Screenshots

Add the following for presentation or submission:
- Web Dashboard screenshots
- Feeding demonstration screenshots
- Serial monitor logs
- GitHub repository link
- Project page link

---

## 🏁 Conclusion

This project demonstrates a **complete IoT system**, combining embedded firmware, networking, and a modern Web UI, implemented entirely in a simulation-first environment.



