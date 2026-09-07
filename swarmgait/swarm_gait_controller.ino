/*
 * swarm_gait_controller.ino
 * SWARM Crab Robot
 * Alternating tetrapod gait state machine.
 *
 * Author : Daniel Huang
 * Program: UCLA ECE Fast-Track B.S., Class of 2030
 *
 * Sequences 16 SES joints (8 legs x 2 joints each) through a
 * 4-phase alternating tetrapod gait. Each joint is switched via
 * an OC100HG optocoupler on a dedicated digital output pin.
 *
 * Gait phases:
 *   Phase 0 : Legs 0,2,4,6 push    | Legs 1,3,5,7 in return stroke
 *   Phase 1 : All joints off        | Full return, all joints reset
 *   Phase 2 : Legs 1,3,5,7 push    | Legs 0,2,4,6 in return stroke
 *   Phase 3 : All joints off        | Full return, all joints reset
 *
 * Phase duration must exceed the SES fall time (31 ms) to prevent
 * joints from stacking activation cycles and drifting out of sync.
 * PHASE_DURATION_MS is set to 50 ms, giving 19 ms of margin.
 *
 * Accepts serial commands from the companion computer (RPi/Jetson)
 * via handleSerial(). See swarm_serial_interface.ino for protocol.
 *
 * Requires: config.h
 */

#include "../config.h"

// Gait table
// Rows = gait phases (0-3). Columns = joint index (0-15).
// 1 = HV on (joint bends), 0 = HV off (elastic hinge returns).
// Column order: L0H L0K  L1H L1K  L2H L2K  L3H L3K
//               L4H L4K  L5H L5K  L6H L6K  L7H L7K
const byte GAIT_TABLE[4][NUM_JOINTS] = {
  {1,1, 0,0, 1,1, 0,0, 1,1, 0,0, 1,1, 0,0},  // Phase 0: even legs push
  {0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0},  // Phase 1: all return
  {0,0, 1,1, 0,0, 1,1, 0,0, 1,1, 0,0, 1,1},  // Phase 2: odd legs push
  {0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0},  // Phase 3: all return
};

int  currentPhase = 0;
bool gaitRunning  = false;

void setup() {
  Serial.begin(SERIAL_BAUD);
  for (int j = 0; j < NUM_JOINTS; j++) {
    pinMode(JOINT_PINS[j], OUTPUT);
    digitalWrite(JOINT_PINS[j], LOW);
  }
  Serial.println("SWARM gait controller ready.");
  Serial.println("Commands: G:forward | G:stop | V:<0.0-1.0> | J:<leg>:<joint>");
}

void loop() {
  handleSerial();
  if (gaitRunning) {
    applyPhase(currentPhase);
    delay(PHASE_DURATION_MS);
    currentPhase = (currentPhase + 1) % 4;
  }
}

// Write one gait phase to all joint pins.
void applyPhase(int phase) {
  for (int j = 0; j < NUM_JOINTS; j++) {
    digitalWrite(JOINT_PINS[j], GAIT_TABLE[phase][j]);
  }
}

// Cut all joints immediately.
// Note: bleed resistors still take ~31 ms to discharge stored charge.
void stopAll() {
  gaitRunning = false;
  for (int j = 0; j < NUM_JOINTS; j++) {
    digitalWrite(JOINT_PINS[j], LOW);
  }
  Serial.println("Stopped. Joints discharging via bleed resistors (~31 ms).");
}

// Returns the flat pin-array index for a given leg and joint.
int jointIndex(int leg, int joint) {
  return (leg * JOINTS_PER_LEG) + joint;
}

// Non-blocking serial command handler.
// Commands are sent by the RPi/Jetson companion computer.
void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "G:forward") {
    gaitRunning  = true;
    currentPhase = 0;
    Serial.println("Gait started.");

  } else if (cmd == "G:stop") {
    stopAll();

  } else if (cmd.startsWith("V:")) {
    // Set HV amplifier output: V:<fraction 0.0-1.0>
    float frac = constrain(cmd.substring(2).toFloat(), 0.0, 1.0);
    analogWrite(HV_CONTROL_PIN, (int)(frac * HV_DEV_MAX_PWM));
    Serial.print("Voltage: "); Serial.print((int)(frac * 100)); Serial.println("%");

  } else if (cmd.startsWith("J:")) {
    // Fire a single joint once: J:<leg>:<joint>
    int sep   = cmd.indexOf(':', 2);
    int leg   = cmd.substring(2, sep).toInt();
    int joint = cmd.substring(sep + 1).toInt();
    int pin   = JOINT_PINS[jointIndex(leg, joint)];
    digitalWrite(pin, HIGH);
    delay(PHASE_DURATION_MS);
    digitalWrite(pin, LOW);
    Serial.print("Fired leg "); Serial.print(leg);
    Serial.print(", joint "); Serial.println(joint);

  } else if (cmd.length() > 0) {
    Serial.print("Unknown: "); Serial.println(cmd);
  }
}
