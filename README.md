# Dip‑Machine‑V1
---
## 🔍 Overview

**Dip‑Machine‑V1** is a precise, ESP32-based dip coating machine designed for automated dipping operations. It enables customizable dip cycles by controlling Dip durattion, stering speed, and cycles of a platform—ideal for lab coating experiments.

![Hardware Setup](docs/images/hardware_setup.jpg)

![Dip Machine 3D Render](docs/images/render.png)

![Wiring Diagram](docs/images/wiring_diagram.png)

This system is controlled via the companion [DipApp](https://github.com/UtkarshJagtap/dipmachin_app), which provides a user-friendly interface to configure dip parameters and control the machine in real-time.

The firmware is modular and divided into components:
- `MachineLink`: Handles motor movement and position control
- `IndicatorLink`: Controls visual indicators (LEDs, etc.)
- `AppLink`: Manages communication between the DipApp and the Arduino
---
## 🔧 What Each Part Does

- **include/Globals.h** – Shared constants, global definitions.
- **src/**  
  - `IndicatorLink`: handles signaling (LEDs, indicators).  
  - `MachineLink`: controls dip mechanism (motor, position sensors).  
  - `AppLink.cpp`: manages communication between Arduino and DipApp.  
  - `main.cpp`: core loop — reads commands, triggers motion, sends status.

---

## ⚙️ How it Works

- **DipApp** sends commands (speed, dip depth, dwell time, start/stop) via Websockets.
- `AppLink.cpp` parses commands and triggers the hardware routines.
- `MachineLink` moves stepper motor and monitors end stops or position.
- `IndicatorLink` signals machine status (running, errors, ready).
- Operational parameters stored, optionally persisted in EEPROM.
- Auto Power loss recovery recovers last running state after power loss

---

## 🚀 Getting Started

1. Install PlatformIO in VS Code or use Arduino IDE (adapt code accordingly).
2. Connect ESP32-compatible board.
3. Wire stepper driver, limit sensors, and indicators as per `Globals.h` and wiring diagrams.
4. Build and upload using PlatformIO (`platformio.ini`).
5. Clone and run DipApp from its repo to configure the machine.
   - Use dummy server mode if hardware isn’t connected.
6. Open DipApp, input your dip values, and hit **Start**.


