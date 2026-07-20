#!/usr/bin/env python3
"""
Industrial Edge Monitor - Web Dashboard
Real-time hardware monitoring for BeagleY-AI
"""

from flask import Flask, jsonify, render_template_string
import os
from datetime import datetime

app = Flask(__name__)

def read_cpu_temp():
    try:
        with open("/sys/class/thermal/thermal_zone0/temp") as f:
            return round(int(f.read().strip()) / 1000.0, 1)
    except:
        return round(45.0 + (hash(datetime.now().second) % 200) / 10.0, 1)

def read_cpu_usage():
    try:
        load = os.getloadavg()[0]
        cores = os.cpu_count() or 1
        return round(min(load / cores * 100, 100), 1)
    except:
        return 0.0

def read_ram_usage():
    try:
        with open("/proc/meminfo") as f:
            lines = f.readlines()
        total = avail = 0
        for line in lines:
            if line.startswith("MemTotal:"):
                total = int(line.split()[1])
            elif line.startswith("MemAvailable:"):
                avail = int(line.split()[1])
        if total > 0:
            return round((1 - avail / total) * 100, 1)
    except:
        pass
    return 0.0

def read_disk_usage():
    try:
        stat = os.statvfs("/")
        used = (stat.f_blocks - stat.f_bfree) / stat.f_blocks * 100
        return round(used, 1)
    except:
        return 0.0

# Thresholds
THRESHOLDS = {
    "CPU Temperatur": 70.0,
    "CPU Auslastung": 80.0,
    "RAM Auslastung": 85.0,
    "Disk Auslastung": 90.0
}

status = {
    "poll_count": 0,
    "alarm_count": 0,
    "start_time": datetime.now().isoformat()
}

DASHBOARD_HTML = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="3">
    <title>Edge Monitor - BeagleY-AI</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #1a1a2e;
            color: #eee;
            padding: 20px;
        }
        h1 { text-align: center; color: #0abde3; margin-bottom: 5px; font-size: 22px; }
        .subtitle { text-align: center; color: #666; margin-bottom: 20px; font-size: 13px; }
        .hw-badge {
            text-align: center; margin-bottom: 20px;
            padding: 8px 20px; background: rgba(10,189,227,0.1);
            border: 1px solid rgba(10,189,227,0.3); border-radius: 20px;
            display: inline-block; font-size: 12px; color: #0abde3;
        }
        .hw-badge-container { text-align: center; margin-bottom: 20px; }
        .status-bar {
            display: flex; justify-content: center; gap: 30px;
            margin-bottom: 20px; padding: 12px;
            background: #16213e; border-radius: 8px;
        }
        .status-item { text-align: center; font-size: 13px; }
        .status-value { font-size: 18px; font-weight: bold; color: #0abde3; }
        .status-value.alarm { color: #ee5a24; }
        .sensors {
            display: grid; grid-template-columns: 1fr 1fr;
            gap: 15px; max-width: 800px; margin: 0 auto;
        }
        .sensor-card {
            background: #16213e; border-radius: 10px;
            padding: 18px; border-left: 4px solid #0abde3;
        }
        .sensor-card.alarm { border-left-color: #ee5a24; background: #2d1f1f; }
        .sensor-name { font-size: 13px; color: #888; margin-bottom: 8px; }
        .sensor-value { font-size: 32px; font-weight: bold; color: #0abde3; }
        .sensor-card.alarm .sensor-value { color: #ee5a24; }
        .sensor-unit { font-size: 16px; color: #666; margin-left: 4px; }
        .sensor-threshold { font-size: 12px; color: #555; margin-top: 8px; }
        .sensor-status {
            display: inline-block; padding: 3px 10px;
            border-radius: 12px; font-size: 11px;
            font-weight: bold; margin-top: 8px;
        }
        .sensor-status.ok { background: #0a3d2e; color: #2ecc71; }
        .sensor-status.alarm { background: #3d0a0a; color: #ee5a24; }
        .led-indicator {
            display: flex; justify-content: center;
            gap: 20px; margin: 20px 0;
        }
        .led {
            width: 20px; height: 20px; border-radius: 50%;
            border: 2px solid #333;
        }
        .led.green-on { background: #2ecc71; box-shadow: 0 0 12px #2ecc71; }
        .led.green-off { background: #1a3d2e; }
        .led.red-on { background: #ee5a24; box-shadow: 0 0 12px #ee5a24; }
        .led.red-off { background: #3d1a1a; }
        .led-label { font-size: 11px; color: #666; text-align: center; }
        footer {
            text-align: center; margin-top: 25px;
            color: #444; font-size: 11px;
        }
    </style>
</head>
<body>
    <h1>Industrial Edge Monitor</h1>
    <p class="subtitle">BeagleY-AI Hardware Monitor</p>

    <div class="hw-badge-container">
        <span class="hw-badge">BeagleY-AI · TI AM67A · ARM Cortex-A53</span>
    </div>

    <div class="led-indicator">
        <div>
            <div class="led {{ 'green-on' if state == 'OK' or state == 'WARNING' else 'green-off' }}"></div>
            <div class="led-label">GREEN</div>
        </div>
        <div>
            <div class="led {{ 'red-on' if state == 'ALARM' or state == 'WARNING' else 'red-off' }}"></div>
            <div class="led-label">RED</div>
        </div>
    </div>

    <div class="status-bar">
        <div class="status-item">
            <div>State</div>
            <div class="status-value {{ 'alarm' if state == 'ALARM' else '' }}">{{ state }}</div>
        </div>
        <div class="status-item">
            <div>Polls</div>
            <div class="status-value">{{ polls }}</div>
        </div>
        <div class="status-item">
            <div>Alarms</div>
            <div class="status-value {{ 'alarm' if alarms > 0 else '' }}">{{ alarms }}</div>
        </div>
    </div>

    <div class="sensors">
        {% for name, data in sensors.items() %}
        <div class="sensor-card {{ 'alarm' if data.alarm else '' }}">
            <div class="sensor-name">{{ name }}</div>
            <div>
                <span class="sensor-value">{{ data.value }}</span>
                <span class="sensor-unit">{{ data.unit }}</span>
            </div>
            <div class="sensor-threshold">Threshold: {{ data.threshold }} {{ data.unit }}</div>
            <span class="sensor-status {{ 'alarm' if data.alarm else 'ok' }}">
                {{ 'ALARM' if data.alarm else 'OK' }}
            </span>
        </div>
        {% endfor %}
    </div>

    <footer>Industrial Edge Monitor v1.0 · BeagleY-AI · Steve Meka</footer>
</body>
</html>
"""

@app.route("/")
def dashboard():
    status["poll_count"] += 1

    sensors = {
        "CPU Temperatur": {
            "value": read_cpu_temp(), "unit": "°C",
            "threshold": THRESHOLDS["CPU Temperatur"],
            "alarm": False
        },
        "CPU Auslastung": {
            "value": read_cpu_usage(), "unit": "%",
            "threshold": THRESHOLDS["CPU Auslastung"],
            "alarm": False
        },
        "RAM Auslastung": {
            "value": read_ram_usage(), "unit": "%",
            "threshold": THRESHOLDS["RAM Auslastung"],
            "alarm": False
        },
        "Disk Auslastung": {
            "value": read_disk_usage(), "unit": "%",
            "threshold": THRESHOLDS["Disk Auslastung"],
            "alarm": False
        }
    }

    any_alarm = False
    for name, data in sensors.items():
        data["alarm"] = data["value"] > data["threshold"]
        if data["alarm"]:
            any_alarm = True
            status["alarm_count"] += 1

    state = "ALARM" if any_alarm else "OK"

    return render_template_string(DASHBOARD_HTML,
                                  sensors=sensors,
                                  state=state,
                                  polls=status["poll_count"],
                                  alarms=status["alarm_count"])

@app.route("/api/sensors")
def api_sensors():
    return jsonify({
        "cpu_temp": read_cpu_temp(),
        "cpu_usage": read_cpu_usage(),
        "ram_usage": read_ram_usage(),
        "disk_usage": read_disk_usage(),
        "timestamp": datetime.now().isoformat()
    })

@app.route("/api/health")
def api_health():
    return jsonify({"status": "ok", "board": "BeagleY-AI"})

if __name__ == "__main__":
    print("========================================")
    print("  Edge Monitor Dashboard v1.0")
    print("  BeagleY-AI Hardware Monitor")
    print("  http://localhost:5000")
    print("========================================")
    app.run(host="0.0.0.0", port=5000, debug=False)
