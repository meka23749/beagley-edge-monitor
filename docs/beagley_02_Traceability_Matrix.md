# Requirement ↔ Test Traceability Matrix

This matrix links each testable requirement to the automated Unity test that
verifies it. The state-decision logic is isolated in a pure function
(`evaluate_state`) so it can be unit-tested; the tests run automatically in CI
on every push.

---

## Traceability matrix

| Requirement | Description | Verifying test (Unity) | Status |
|---|---|---|---|
| REQ-02 | All metrics low → state OK | `test_all_low_is_ok` | ✅ Verified |
| REQ-03 | High CPU temperature → ALARM | `test_high_temp_triggers_alarm` | ✅ Verified |
| REQ-04 | High CPU usage → ALARM | `test_high_cpu_triggers_alarm` | ✅ Verified |
| REQ-05 | High RAM usage → ALARM | `test_high_ram_triggers_alarm` | ✅ Verified |
| REQ-06 | Metric near threshold → WARNING | `test_temp_near_threshold_triggers_warning`, `test_cpu_near_threshold_triggers_warning` | ✅ Verified |
| REQ-07 | ALARM has priority over WARNING | `test_alarm_has_priority_over_warning` | ✅ Verified |

---

## Requirements verified by other means

| Requirement | Verification |
|---|---|
| REQ-01 (read real hardware) | Manual / on-target: runs on real BeagleY-AI, reads sysfs & /proc |
| REQ-08 (LED reflects state) | On-target: onboard LEDs via sysfs |
| REQ-09 (LEDs off on shutdown) | On-target: signal handler tested manually |
| REQ-10 (MQTT / REST publish) | Dashboard API tested in CI (`Test Web Dashboard` job) |

---

## Coverage summary

- **6 logic requirements verified by 7 automated unit tests.**
- Hardware/integration requirements (REQ-01, 08, 09) verified on the real target.
- All unit tests run automatically in CI (see the `Unit Tests (logic)` job).

