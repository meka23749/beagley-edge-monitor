# Requirements Specification

---

## 1. Purpose

This document specifies the functional and monitoring requirements of the Edge
Monitor firmware running on a BeagleY-AI board under a custom Yocto Linux image.
The monitor reads real system metrics (temperature, CPU, RAM, disk, load),
determines an overall system state, drives onboard LEDs, and publishes data via
MQTT / REST.

The state-decision logic is hardware-independent and verified by automated unit
tests (Unity), executed in CI.

---

## 2. Definitions

| Term | Meaning |
|---|---|
| Metric | A measured system value (temperature, CPU %, RAM %, …) |
| State | Overall monitor state: OK, WARNING, ALARM, ERROR |
| Alarm threshold | Value above which a metric is critical |
| Warning ratio | Fraction of the alarm threshold that triggers a warning |

---

## 3. Functional requirements

| ID | Requirement | Value |
|---|---|---|
| REQ-01 | The system shall read real hardware metrics from the Linux interfaces (thermal, /proc, statvfs). | sysfs, /proc |
| REQ-02 | The system shall report state OK when all metrics are below the warning levels. | all normal |
| REQ-03 | The system shall report state ALARM when CPU temperature exceeds its alarm threshold. | > 70 °C |
| REQ-04 | The system shall report state ALARM when CPU usage exceeds its alarm threshold. | > 80 % |
| REQ-05 | The system shall report state ALARM when RAM usage exceeds its alarm threshold. | > 85 % |
| REQ-06 | The system shall report state WARNING when a metric approaches its alarm threshold. | > 85 % of threshold |
| REQ-07 | ALARM shall take priority over WARNING when both conditions are met. | ALARM wins |

---

## 4. Behavioral requirements

| ID | Requirement |
|---|---|
| REQ-08 | The onboard LEDs shall reflect the current state (e.g. green = OK, red = ALARM). |
| REQ-09 | On shutdown, the LEDs shall be turned off (clean stop via signal handler). |
| REQ-10 | Sensor data shall be published via MQTT and exposed through a REST API. |

---

## 5. Design notes

- Thresholds are defined in `monitor_logic.h`
  (`TEMP_ALARM_C = 70`, `CPU_ALARM_PERCENT = 80`, `RAM_ALARM_PERCENT = 85`,
  `WARNING_RATIO = 0.85`).
- The state-decision logic is isolated in `monitor_logic.c` as a pure function
  `evaluate_state(system_metrics_t)` — no I/O — so it is unit-testable on a host PC.
- The hardware reading (real sensors) stays in `edge_monitor.c`; the project keeps
  its real-hardware value while the decision logic becomes fully testable.
- Each testable requirement is verified by an automated test — see
  [02_Traceability_Matrix.md](02_Traceability_Matrix.md).
