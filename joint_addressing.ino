// ============================================================
//  joint_addressing.ino  —  Joint Pin Map & Index Helpers
//  SWARM Crab Robot  |  SES Electrohydraulic Joint System
//  Author: Daniel Huang  |  UCLA ECE Fast-Track 2030
//
//  Maps (leg, joint) tuples to Arduino pin indices.
//  All 16 OC100HG optocoupler channels are registered here.
//  Include this file alongside gait_state_machine.ino.
// ============================================================

#include "../config.h"

// ── Returns flat pin index for a given leg + joint ───────────
//
//  leg   : 0–7  (clockwise from front-left)
//  joint : HIP (0) or KNEE (1)
//
//  Example:
//    jointIndex(3, KNEE) → pin index 7 → JOINT_PINS[7] = 9
//
int jointIndex(int leg, int joint) {
  return (leg * JOINTS_PER_LEG) + joint;
}

// ── Initialise all joint pins as outputs, default LOW (HV off)
void initJoints() {
  for (int j = 0; j < NUM_JOINTS; j++) {
    pinMode(JOINT_PINS[j], OUTPUT);
    digitalWrite(JOINT_PINS[j], LOW);
  }
}

// ── Set a single joint HIGH (HV on) or LOW (HV off) ─────────
void setJoint(int leg, int joint, bool state) {
  digitalWrite(JOINT_PINS[jointIndex(leg, joint)], state ? HIGH : LOW);
}

// ── Cut all joints immediately — emergency stop ───────────────
//  Call this before any maintenance or if a fault is detected.
//  NOTE: bleed resistors still take ~31 ms to fully discharge.
void stopAll() {
  for (int j = 0; j < NUM_JOINTS; j++) {
    digitalWrite(JOINT_PINS[j], LOW);
  }
}
