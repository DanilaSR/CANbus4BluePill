#!/usr/bin/env python3
"""Real-time MPU9250 data visualizer.

Reads CSV stream (ax,ay,az,gx,gy,gz,temp) from /dev/ttyUSB0
and plots it live with matplotlib.

Usage: python3 visualize.py [port] [baud]
       Close tio first!
"""

import sys
import serial
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from collections import deque

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
WINDOW = 200
LABELS = ["ax", "ay", "az", "gx", "gy", "gz", "temp"]
COLORS = ["#e74c3c", "#2ecc71", "#3498db", "#e67e22", "#9b59b6", "#1abc9c", "#95a5a6"]

ser = serial.Serial(PORT, BAUD, timeout=1)

buf = {k: deque(maxlen=WINDOW) for k in LABELS}
ticks = deque(maxlen=WINDOW)
tick = 0

fig, (ax_acc, ax_gyro, ax_temp) = plt.subplots(3, 1, figsize=(10, 7), sharex=True,
    gridspec_kw={"height_ratios": [3, 3, 1]})
fig.suptitle("MPU9250 Live Stream", fontsize=13)

lines_acc = [ax_acc.plot([], [], label=l, color=c, linewidth=1)[0] for l, c in zip(LABELS[:3], COLORS[:3])]
ax_acc.set_ylabel("Acceleration (g)")
ax_acc.legend(loc="upper left", fontsize=8)
ax_acc.grid(True, alpha=0.3)

lines_gyro = [ax_gyro.plot([], [], label=l, color=c, linewidth=1)[0] for l, c in zip(LABELS[3:6], COLORS[3:6])]
ax_gyro.set_ylabel("Gyro (dps)")
ax_gyro.legend(loc="upper left", fontsize=8)
ax_gyro.grid(True, alpha=0.3)

line_temp, = ax_temp.plot([], [], color=COLORS[6], linewidth=1)
ax_temp.set_ylabel("Temp (\u00b0C)")
ax_temp.set_xlabel("Sample")
ax_temp.grid(True, alpha=0.3)

fig.tight_layout()
plt.ion()
plt.show()

print(f"Reading from {PORT} @ {BAUD} baud. Close window or Ctrl+C to stop.")

try:
    while plt.fignum_exists(fig.number):
        raw = ser.readline()
        if not raw:
            plt.pause(0.01)
            continue
        line = raw.decode("ascii", errors="ignore").strip()
        parts = line.split(",")
        if len(parts) != 7:
            continue
        try:
            vals = [float(p) for p in parts]
        except ValueError:
            continue
        for i, k in enumerate(LABELS):
            buf[k].append(vals[i])
        ticks.append(tick)
        tick += 1

        if tick % 5 == 0:
            for i, ln in enumerate(lines_acc):
                ln.set_data(list(ticks), list(buf[LABELS[i]]))
            for i, ln in enumerate(lines_gyro):
                ln.set_data(list(ticks), list(buf[LABELS[i + 3]]))
            line_temp.set_data(list(ticks), list(buf["temp"]))
            for a in (ax_acc, ax_gyro, ax_temp):
                a.relim()
                a.autoscale_view()
            fig.canvas.draw_idle()
            fig.canvas.flush_events()
except KeyboardInterrupt:
    pass
finally:
    ser.close()
    plt.ioff()
    plt.close()
    print("Done.")
