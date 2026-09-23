#include "monitor_logic.h"

monitor_state_t evaluate_state(system_metrics_t m) {
    /* ALARM: any critical threshold exceeded */
    if (m.cpu_temp  > TEMP_ALARM_C ||
        m.cpu_usage > CPU_ALARM_PERCENT ||
        m.ram_usage > RAM_ALARM_PERCENT) {
        return STATE_ALARM;
    }

    /* WARNING: approaching the alarm threshold */
    if (m.cpu_temp  > (TEMP_ALARM_C * WARNING_RATIO) ||
        m.cpu_usage > (CPU_ALARM_PERCENT * WARNING_RATIO)) {
        return STATE_WARNING;
    }

    /* Otherwise everything is OK */
    return STATE_OK;
}