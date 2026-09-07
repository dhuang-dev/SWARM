/*
 * swarm_serial_interface.ino
 * SWARM Crab Robot
 * Serial command bridge between companion computer and Arduino.
 *
 * Author : Daniel Huang
 * Program: UCLA ECE Fast-Track B.S., Class of 2030
 *
 * The Raspberry Pi / Jetson Nano runs SLAM, AI, and networking.
 * It sends high-level commands over USB serial, and the Arduino
 * executes them in hard real time on the actuation hardware.
 *
 * This separation keeps all hard real-time constraints on the
 * Arduino (actuation timing, gait sequencing) and all compute-
 * intensive tasks on the companion computer. A SLAM computation
 * spike on the RPi cannot cause a gait timing failure.
 *
 * Command protocol (9600 baud, newline terminated):
 *
 *   G:forward          Start forward alternating tetrapod gait
 *   G:stop             Halt all joints immediately
 *   V:<0.0-1.0>        Set HV fraction (e.g. V:0.75 = 75% of max)
 *   J:<leg>:<joint>    Fire one joint for one phase duration
 *                      leg = 0-7, joint = 0 (hip) or 1 (knee)
 *   STATUS             Print current gait phase and voltage setting
 *   PING               Responds PONG (connection check)
 *
 * Response format: plain ASCII, newline terminated.
 * The companion computer checks for "ERR" prefix to detect faults.
 *
 * Requires: config.h
 */

#include "../config.h"

// Shared state (also modified by gait controller)
extern bool gaitRunning;
extern int  currentPhase;

float currentVoltageFraction = 0.0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(HV_CONTROL_PIN, OUTPUT);
  analogWrite(HV_CONTROL_PIN, 0);
  for (int j = 0; j < NUM_JOINTS; j++) {
    pinMode(JOINT_PINS[j], OUTPUT);
    digitalWrite(JOINT_PINS[j], LOW);
  }
  Serial.println("SWARM serial interface ready.");
}

void loop() {
  handleSerial();
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "G:forward") {
    gaitRunning  = true;
    currentPhase = 0;
    Serial.println("OK: gait started");

  } else if (cmd == "G:stop") {
    gaitRunning = false;
    for (int j = 0; j < NUM_JOINTS; j++) digitalWrite(JOINT_PINS[j], LOW);
    Serial.println("OK: stopped");

  } else if (cmd.startsWith("V:")) {
    float frac = constrain(cmd.substring(2).toFloat(), 0.0, 1.0);
    currentVoltageFraction = frac;
    analogWrite(HV_CONTROL_PIN, (int)(frac * HV_DEV_MAX_PWM));
    Serial.print("OK: voltage "); Serial.println(frac, 2);

  } else if (cmd.startsWith("J:")) {
    int sep   = cmd.indexOf(':', 2);
    if (sep < 0) { Serial.println("ERR: bad J format, use J:<leg>:<joint>"); return; }
    int leg   = cmd.substring(2, sep).toInt();
    int joint = cmd.substring(sep + 1).toInt();
    if (leg < 0 || leg >= NUM_LEGS || joint < 0 || joint >= JOINTS_PER_LEG) {
      Serial.println("ERR: leg 0-7, joint 0-1"); return;
    }
    int pin = JOINT_PINS[leg * JOINTS_PER_LEG + joint];
    digitalWrite(pin, HIGH);
    delay(PHASE_DURATION_MS);
    digitalWrite(pin, LOW);
    Serial.print("OK: fired leg "); Serial.print(leg);
    Serial.print(" joint "); Serial.println(joint);

  } else if (cmd == "STATUS") {
    Serial.print("gait=");     Serial.println(gaitRunning ? "running" : "stopped");
    Serial.print("phase=");    Serial.println(currentPhase);
    Serial.print("voltage=");  Serial.println(currentVoltageFraction, 2);

  } else if (cmd == "PING") {
    Serial.println("PONG");

  } else if (cmd.length() > 0) {
    Serial.print("ERR: unknown command: "); Serial.println(cmd);
    Serial.println("Commands: G:forward | G:stop | V:<f> | J:<l>:<j> | STATUS | PING");
  }
}
