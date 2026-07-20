# Industrial Edge Monitor (BeagleY-AI)

Real-time hardware monitoring system running on BeagleY-AI with a custom Yocto Linux image.

## What it does
- Reads real CPU temperature from the SoC thermal sensor
- Monitors system resources (CPU load, RAM, disk)
- Controls onboard LEDs based on system state (OK/ALARM)
- Publishes sensor data via MQTT
- Serves a real-time web dashboard

## Hardware
- **Board**: BeagleY-AI (TI AM67A, ARM Cortex-A53)
- **Sensors**: Onboard thermal sensor, system metrics
- **Actuators**: Onboard User LEDs
- **Network**: WiFi / Ethernet

## Difference from QEMU project
| | QEMU Project | This Project |
|---|---|---|
| Hardware | Emulated ARM64 | Real BeagleY-AI |
| Temperature | Simulated | Real SoC sensor |
| LEDs | Simulated (printf) | Real physical LEDs |
| Network | Virtual | Real WiFi/Ethernet |
| Dashboard | localhost | Accessible from any device |

## Tech Stack
- **OS**: Custom Yocto Linux (Scarthgap) for BeagleY-AI
- **Application**: C (sensor reading, LED control, MQTT)
- **Dashboard**: Python Flask
- **Build**: Dockerized Yocto build
- **CI/CD**: GitHub Actions

## Project Status
🚧 Work in progress

## Author
Steve Meka — Embedded Software Engineer
