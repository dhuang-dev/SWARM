"""
swarm_companion.py
SWARM Crab Robot
Companion computer serial bridge and SLAM command dispatcher.

Author : Daniel Huang
Program: UCLA ECE Fast-Track B.S., Class of 2030

Runs on the Raspberry Pi or Jetson Nano. Connects to the Arduino
over USB serial and issues gait/voltage commands based on SLAM
output and AI navigation decisions.

Architecture:
  - Arduino owns all hard real-time actuation (gait phases, joint switching)
  - This script owns SLAM processing, AI inference, and mesh networking
  - Commands flow: SLAM -> decision logic -> serial -> Arduino
  - Responses flow: Arduino -> serial -> this script -> log/UI

Usage:
  python3 swarm_companion.py --port /dev/ttyUSB0 --baud 9600

Dependencies:
  pip install pyserial

Optional (for full SLAM/ROS integration):
  pip install rclpy  (ROS2 Python client)
"""

import serial
import time
import argparse
import threading
import sys


# ── Configuration ────────────────────────────────────────────
DEFAULT_PORT  = "/dev/ttyUSB0"
DEFAULT_BAUD  = 9600
CONNECT_DELAY = 2.0   # seconds: wait for Arduino to finish reset


# ── SwarmBridge ──────────────────────────────────────────────

class SwarmBridge:
    """
    Serial bridge to the Arduino gait controller.
    Thread-safe: a background thread reads responses continuously.
    """

    def __init__(self, port: str, baud: int):
        self.port = port
        self.baud = baud
        self.ser  = None
        self._running = False

    def connect(self) -> bool:
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            time.sleep(CONNECT_DELAY)   # let Arduino complete reset
            self._running = True
            t = threading.Thread(target=self._read_loop, daemon=True)
            t.start()
            print(f"[bridge] connected on {self.port} at {self.baud} baud")
            return True
        except serial.SerialException as e:
            print(f"[bridge] connection failed: {e}")
            return False

    def disconnect(self):
        self._running = False
        if self.ser and self.ser.is_open:
            self.send("G:stop")
            time.sleep(0.1)
            self.ser.close()
        print("[bridge] disconnected")

    def send(self, cmd: str):
        """Send a newline-terminated command string to the Arduino."""
        if not self.ser or not self.ser.is_open:
            print("[bridge] not connected")
            return
        self.ser.write((cmd.strip() + "\n").encode("utf-8"))

    def _read_loop(self):
        """Background thread: print all Arduino responses to stdout."""
        while self._running:
            try:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        print(f"[arduino] {line}")
            except serial.SerialException:
                break


# ── High-level gait commands ─────────────────────────────────

class SwarmController:
    """
    High-level controller. Translates navigation decisions into
    Arduino serial commands via SwarmBridge.
    """

    def __init__(self, bridge: SwarmBridge):
        self.bridge = bridge

    def start_forward(self):
        print("[ctrl] starting forward gait")
        self.bridge.send("G:forward")

    def stop(self):
        print("[ctrl] stopping")
        self.bridge.send("G:stop")

    def set_voltage(self, fraction: float):
        """Set HV fraction 0.0 (off) to 1.0 (max dev ceiling ~6.3 kV)."""
        fraction = max(0.0, min(1.0, fraction))
        print(f"[ctrl] voltage -> {fraction:.2f}")
        self.bridge.send(f"V:{fraction:.2f}")

    def fire_joint(self, leg: int, joint: int):
        """Manually fire one joint for one phase duration."""
        print(f"[ctrl] firing leg {leg} joint {joint}")
        self.bridge.send(f"J:{leg}:{joint}")

    def ping(self) -> None:
        self.bridge.send("PING")

    def status(self) -> None:
        self.bridge.send("STATUS")


# ── Demo sequence ────────────────────────────────────────────

def run_demo(ctrl: SwarmController):
    """
    Basic demo: ramp voltage, start gait for 5 seconds, stop.
    Replace this with SLAM-driven navigation in production.
    """
    print("[demo] ramping voltage to 70%")
    ctrl.set_voltage(0.7)
    time.sleep(0.6)   # allow RC filter to settle (~500 ms)

    print("[demo] starting gait")
    ctrl.start_forward()
    time.sleep(5.0)

    print("[demo] stopping")
    ctrl.stop()
    ctrl.set_voltage(0.0)


# ── Entry point ──────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="SWARM companion computer bridge")
    parser.add_argument("--port",  default=DEFAULT_PORT, help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("--baud",  default=DEFAULT_BAUD, type=int)
    parser.add_argument("--demo",  action="store_true", help="Run demo gait sequence and exit")
    args = parser.parse_args()

    bridge = SwarmBridge(args.port, args.baud)
    if not bridge.connect():
        sys.exit(1)

    ctrl = SwarmController(bridge)
    ctrl.ping()
    time.sleep(0.1)

    try:
        if args.demo:
            run_demo(ctrl)
        else:
            # Interactive mode: type commands directly
            print("Interactive mode. Type commands (G:forward, G:stop, V:0.75, quit):")
            while True:
                raw = input("> ").strip()
                if raw.lower() in ("quit", "exit", "q"):
                    break
                bridge.send(raw)
    except KeyboardInterrupt:
        pass
    finally:
        bridge.disconnect()


if __name__ == "__main__":
    main()
