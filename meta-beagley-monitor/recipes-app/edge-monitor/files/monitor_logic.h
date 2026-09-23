#ifndef MONITOR_LOGIC_H
#define MONITOR_LOGIC_H

/* ===== Thresholds (requirements) ===== */
#define TEMP_ALARM_C        70.0f
#define CPU_ALARM_PERCENT   80.0f
#define RAM_ALARM_PERCENT   85.0f
#define DISK_ALARM_PERCENT  90.0f
#define WARNING_RATIO       0.85f   /* WARNING at 85% of the alarm threshold */

/* ===== Metrics (pure data, no I/O) ===== */
typedef struct {
    float cpu_temp;
    float cpu_usage;
    float ram_usage;
    float disk_usage;
    float load_avg_1min;
} system_metrics_t;

/* ===== Monitor state ===== */
typedef enum {
    STATE_OK      = 0,
    STATE_WARNING = 1,
    STATE_ALARM   = 2,
    STATE_ERROR   = 3
} monitor_state_t;

/* Pure decision function: given metrics, return the monitor state.
 * No file reading, no side effects -> fully testable. */
monitor_state_t evaluate_state(system_metrics_t metrics);

#endif