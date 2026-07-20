#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>

/*
 * Industrial Edge Monitor - BeagleY-AI
 * Reads REAL hardware sensors and controls REAL LEDs
 */

/* ===== CONFIGURATION ===== */
#define POLL_INTERVAL_SEC   2
#define TEMP_ALARM_C        70.0f
#define CPU_ALARM_PERCENT   80.0f
#define RAM_ALARM_PERCENT   85.0f
#define DISK_ALARM_PERCENT  90.0f
#define MAX_HISTORY         30

/* ===== DATA STRUCTURES ===== */
typedef struct {
    float cpu_temp;
    float cpu_usage;
    float ram_usage;
    float disk_usage;
    float load_avg_1min;
} system_metrics_t;

typedef enum {
    STATE_OK      = 0,
    STATE_WARNING = 1,
    STATE_ALARM   = 2,
    STATE_ERROR   = 3
} monitor_state_t;

typedef struct {
    monitor_state_t state;
    system_metrics_t current;
    system_metrics_t history[MAX_HISTORY];
    uint8_t          history_index;
    uint8_t          history_count;
    uint32_t         poll_count;
    uint32_t         alarm_count;
    uint32_t         mqtt_sent;
    time_t           start_time;
    bool             running;
    bool             led_green;
    bool             led_red;
} monitor_t;

static monitor_t monitor;

/* ===== SIGNAL HANDLER ===== */
void signal_handler(int sig) {
    printf("\n[MONITOR] Signal %d received, shutting down...\n", sig);
    monitor.running = false;
}

/* ===== TIMESTAMP ===== */
void get_timestamp(char* buf, size_t len) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

/* ===== READ REAL CPU TEMPERATURE ===== */
float read_cpu_temperature(void) {
    FILE* f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) {
        /* Fallback: simulation if sysfs not available (QEMU) */
        return 45.0f + (float)(rand() % 200) / 10.0f;
    }
    int raw;
    if (fscanf(f, "%d", &raw) != 1) {
        fclose(f);
        return -1.0f;
    }
    fclose(f);
    /* kernel reports in millidegrees: 45000 = 45.0°C */
    return raw / 1000.0f;
}

/* ===== READ REAL CPU USAGE ===== */
static long prev_idle = 0, prev_total = 0;

float read_cpu_usage(void) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return -1.0f;

    char label[16];
    long user, nice, system, idle, iowait, irq, softirq;

    if (fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld",
               label, &user, &nice, &system, &idle,
               &iowait, &irq, &softirq) != 8) {
        fclose(f);
        return -1.0f;
    }
    fclose(f);

    long total = user + nice + system + idle + iowait + irq + softirq;
    long diff_idle = idle - prev_idle;
    long diff_total = total - prev_total;

    float usage = 0.0f;
    if (diff_total > 0) {
        usage = (1.0f - (float)diff_idle / (float)diff_total) * 100.0f;
    }

    prev_idle = idle;
    prev_total = total;

    return usage;
}

/* ===== READ REAL RAM USAGE ===== */
float read_ram_usage(void) {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return -1.0f;

    long mem_total = 0, mem_available = 0;
    char line[128];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld", &mem_total);
        }
        if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line, "MemAvailable: %ld", &mem_available);
        }
    }
    fclose(f);

    if (mem_total == 0) return -1.0f;
    return (1.0f - (float)mem_available / (float)mem_total) * 100.0f;
}

/* ===== READ REAL DISK USAGE ===== */
float read_disk_usage(void) {
    FILE* f = popen("df / | tail -1 | awk '{print $5}' | tr -d '%'", "r");
    if (!f) return -1.0f;

    int usage;
    if (fscanf(f, "%d", &usage) != 1) {
        pclose(f);
        return -1.0f;
    }
    pclose(f);
    return (float)usage;
}

/* ===== READ LOAD AVERAGE ===== */
float read_load_average(void) {
    FILE* f = fopen("/proc/loadavg", "r");
    if (!f) return -1.0f;

    float load;
    if (fscanf(f, "%f", &load) != 1) {
        fclose(f);
        return -1.0f;
    }
    fclose(f);
    return load;
}

/* ===== CONTROL REAL LEDs ===== */
/*
 * BeagleY-AI has user LEDs controllable via sysfs
 * Path: /sys/class/leds/
 *
 * If sysfs LEDs not available, falls back to printf
 */
void set_led(const char* led_name, bool on) {
    char path[128];
    snprintf(path, sizeof(path),
             "/sys/class/leds/%s/brightness", led_name);

    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d", on ? 1 : 0);
        fclose(f);
    }
}

void update_leds(monitor_t* m) {
    switch (m->state) {
        case STATE_OK:
            m->led_green = true;
            m->led_red = false;
            break;
        case STATE_WARNING:
            m->led_green = true;
            m->led_red = true;
            break;
        case STATE_ALARM:
        case STATE_ERROR:
            m->led_green = false;
            m->led_red = true;
            break;
    }

    /* Try to control real LEDs */
    set_led("beaglebone:green:usr0", m->led_green);
    set_led("beaglebone:green:usr1", m->led_red);

    /* Always log LED state */
    printf("[LED] GREEN=%s RED=%s\n",
           m->led_green ? "ON" : "OFF",
           m->led_red ? "ON" : "OFF");
}

/* ===== SIMULATED MQTT PUBLISH ===== */
void mqtt_publish(const char* topic, const char* payload) {
    printf("[MQTT] %s | %s\n", topic, payload);
    monitor.mqtt_sent++;
}

/* ===== LIST AVAILABLE THERMAL ZONES ===== */
void list_thermal_zones(void) {
    FILE* f = fopen("/sys/class/thermal/thermal_zone0/type", "r");
    if (f) {
        char type[64] = {0};
        if (fgets(type, sizeof(type), f)) {
            type[strcspn(type, "\n")] = 0;
            printf("[INFO] thermal_zone0: %s\n", type);
        }
        fclose(f);
    } else {
        printf("[INFO] No thermal zones found (WSL/QEMU mode)\n");
    }
}

/* ===== MONITOR FUNCTIONS ===== */
void monitor_init(monitor_t* m) {
    memset(m, 0, sizeof(monitor_t));
    m->state = STATE_OK;
    m->running = true;
    m->start_time = time(NULL);

    printf("========================================\n");
    printf("  Industrial Edge Monitor v1.0\n");
    printf("  BeagleY-AI Hardware Monitor\n");
    printf("========================================\n\n");

    list_thermal_zones();
    printf("\n");

    /* First CPU reading to initialize counters */
    read_cpu_usage();
}

void monitor_poll(monitor_t* m) {
    system_metrics_t* c = &m->current;
    char topic[128], payload[256];
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Read ALL real sensors */
    c->cpu_temp      = read_cpu_temperature();
    c->cpu_usage     = read_cpu_usage();
    c->ram_usage     = read_ram_usage();
    c->disk_usage    = read_disk_usage();
    c->load_avg_1min = read_load_average();

    /* Determine state */
    if (c->cpu_temp > TEMP_ALARM_C ||
        c->cpu_usage > CPU_ALARM_PERCENT ||
        c->ram_usage > RAM_ALARM_PERCENT) {
        m->state = STATE_ALARM;
        m->alarm_count++;
    } else if (c->cpu_temp > (TEMP_ALARM_C * 0.85f) ||
               c->cpu_usage > (CPU_ALARM_PERCENT * 0.85f)) {
        m->state = STATE_WARNING;
    } else {
        m->state = STATE_OK;
    }

    /* Update LEDs */
    update_leds(m);

    /* Save to history */
    m->history[m->history_index] = *c;
    m->history_index = (m->history_index + 1) % MAX_HISTORY;
    if (m->history_count < MAX_HISTORY) m->history_count++;

    /* Publish via MQTT */
    snprintf(topic, sizeof(topic), "edge/beagley/temperature");
    snprintf(payload, sizeof(payload),
             "{\"value\":%.1f,\"unit\":\"C\",\"alarm\":%s,\"ts\":\"%s\"}",
             c->cpu_temp,
             c->cpu_temp > TEMP_ALARM_C ? "true" : "false",
             timestamp);
    mqtt_publish(topic, payload);

    snprintf(topic, sizeof(topic), "edge/beagley/cpu");
    snprintf(payload, sizeof(payload),
             "{\"value\":%.1f,\"unit\":\"%%\",\"load\":%.2f,\"ts\":\"%s\"}",
             c->cpu_usage, c->load_avg_1min, timestamp);
    mqtt_publish(topic, payload);

    snprintf(topic, sizeof(topic), "edge/beagley/memory");
    snprintf(payload, sizeof(payload),
             "{\"value\":%.1f,\"unit\":\"%%\",\"ts\":\"%s\"}",
             c->ram_usage, timestamp);
    mqtt_publish(topic, payload);

    snprintf(topic, sizeof(topic), "edge/beagley/disk");
    snprintf(payload, sizeof(payload),
             "{\"value\":%.1f,\"unit\":\"%%\",\"ts\":\"%s\"}",
             c->disk_usage, timestamp);
    mqtt_publish(topic, payload);

    m->poll_count++;
}

void monitor_print_status(monitor_t* m) {
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    time_t uptime = time(NULL) - m->start_time;

    const char* state_str;
    switch (m->state) {
        case STATE_OK:      state_str = "OK";      break;
        case STATE_WARNING: state_str = "WARNING";  break;
        case STATE_ALARM:   state_str = "ALARM";    break;
        case STATE_ERROR:   state_str = "ERROR";    break;
        default:            state_str = "UNKNOWN";  break;
    }

    printf("\n--- Edge Monitor [%s] ---\n", timestamp);
    printf("State: %-8s | Polls: %u | Alarms: %u | MQTT: %u | Up: %lds\n",
           state_str, m->poll_count, m->alarm_count,
           m->mqtt_sent, (long)uptime);
    printf("  CPU Temp       : %6.1f C     [max: %.0f C]\n",
           m->current.cpu_temp, TEMP_ALARM_C);
    printf("  CPU Usage      : %6.1f %%     [max: %.0f %%]\n",
           m->current.cpu_usage, CPU_ALARM_PERCENT);
    printf("  RAM Usage      : %6.1f %%     [max: %.0f %%]\n",
           m->current.ram_usage, RAM_ALARM_PERCENT);
    printf("  Disk Usage     : %6.1f %%     [max: %.0f %%]\n",
           m->current.disk_usage, DISK_ALARM_PERCENT);
    printf("  Load Avg (1m)  : %6.2f\n",
           m->current.load_avg_1min);
    printf("--------------------------------\n\n");
}

/* ===== MAIN ===== */
int main(int argc, char* argv[]) {
    int max_cycles = 5;
    if (argc > 1) max_cycles = atoi(argv[1]);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL));
    monitor_init(&monitor);

    int cycle = 0;
    while (monitor.running && cycle < max_cycles) {
        monitor_poll(&monitor);
        monitor_print_status(&monitor);
        cycle++;

        if (cycle < max_cycles && monitor.running) {
            sleep(POLL_INTERVAL_SEC);
        }
    }

    /* Shutdown: turn off LEDs */
    set_led("beaglebone:green:usr0", false);
    set_led("beaglebone:green:usr1", false);

    printf("\n========================================\n");
    printf("  Edge Monitor Shutdown Report\n");
    printf("  Total polls:  %u\n", monitor.poll_count);
    printf("  Total alarms: %u\n", monitor.alarm_count);
    printf("  MQTT messages: %u\n", monitor.mqtt_sent);
    printf("========================================\n");

    return 0;
}
