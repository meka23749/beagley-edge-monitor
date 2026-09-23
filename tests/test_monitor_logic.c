#include "unity.h"
#include "monitor_logic.h"

void setUp(void) {}
void tearDown(void) {}

/* Helper: build a metrics struct with all values low (safe) */
static system_metrics_t safe_metrics(void) {
    system_metrics_t m = {0};
    m.cpu_temp = 40.0f;
    m.cpu_usage = 10.0f;
    m.ram_usage = 20.0f;
    m.disk_usage = 30.0f;
    m.load_avg_1min = 0.5f;
    return m;
}

void test_all_low_is_ok(void) {
    system_metrics_t m = safe_metrics();
    TEST_ASSERT_EQUAL(STATE_OK, evaluate_state(m));
}

void test_high_temp_triggers_alarm(void) {
    system_metrics_t m = safe_metrics();
    m.cpu_temp = 75.0f;   /* > 70 */
    TEST_ASSERT_EQUAL(STATE_ALARM, evaluate_state(m));
}

void test_high_cpu_triggers_alarm(void) {
    system_metrics_t m = safe_metrics();
    m.cpu_usage = 85.0f;  /* > 80 */
    TEST_ASSERT_EQUAL(STATE_ALARM, evaluate_state(m));
}

void test_high_ram_triggers_alarm(void) {
    system_metrics_t m = safe_metrics();
    m.ram_usage = 90.0f;  /* > 85 */
    TEST_ASSERT_EQUAL(STATE_ALARM, evaluate_state(m));
}

void test_temp_near_threshold_triggers_warning(void) {
    system_metrics_t m = safe_metrics();
    m.cpu_temp = 65.0f;   /* > 70*0.85=59.5 but < 70 */
    TEST_ASSERT_EQUAL(STATE_WARNING, evaluate_state(m));
}

void test_cpu_near_threshold_triggers_warning(void) {
    system_metrics_t m = safe_metrics();
    m.cpu_usage = 70.0f;  /* > 80*0.85=68 but < 80 */
    TEST_ASSERT_EQUAL(STATE_WARNING, evaluate_state(m));
}

void test_alarm_has_priority_over_warning(void) {
    system_metrics_t m = safe_metrics();
    m.cpu_temp = 75.0f;   /* alarm level */
    m.cpu_usage = 70.0f;  /* warning level */
    TEST_ASSERT_EQUAL(STATE_ALARM, evaluate_state(m));  /* alarm wins */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_low_is_ok);
    RUN_TEST(test_high_temp_triggers_alarm);
    RUN_TEST(test_high_cpu_triggers_alarm);
    RUN_TEST(test_high_ram_triggers_alarm);
    RUN_TEST(test_temp_near_threshold_triggers_warning);
    RUN_TEST(test_cpu_near_threshold_triggers_warning);
    RUN_TEST(test_alarm_has_priority_over_warning);
    return UNITY_END();
}