/*
 * config.h
 * SWARM Crab Robot
 * Shared hardware configuration for all firmware modules.
 *
 * Author : Daniel Huang
 * Program: UCLA ECE Fast-Track B.S., Class of 2030
 *
 * Edit pin assignments, timing constants, and voltage limits here.
 * All .ino files include this header.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Robot geometry
#define NUM_LEGS          8     // crab morphology
#define JOINTS_PER_LEG    2     // hip + knee
#define NUM_JOINTS        16    // 8 legs x 2 joints
#define HIP               0
#define KNEE              1

// Pin map: Arduino digital/analog pins drive OC100HG optocoupler LEDs.
// Each optocoupler switches HV to one SES joint.
// Legs are numbered 0-7 clockwise from front-left.
// JOINT_PINS[leg * 2 + 0] = hip pin
// JOINT_PINS[leg * 2 + 1] = knee pin
const int JOINT_PINS[NUM_JOINTS] = {
   2,  3,   // Leg 0
   4,  5,   // Leg 1
   6,  7,   // Leg 2
   8,  9,   // Leg 3
  10, 11,   // Leg 4
  12, 13,   // Leg 5
  A0, A1,   // Leg 6
  A2, A3    // Leg 7
};

// HV amplifier control
// PWM output goes through a 10kOhm + 10uF RC filter (tau = 100 ms),
// then into the EMCO G-series input (0-5V maps to 0-9 kV).
#define HV_CONTROL_PIN    9       // must be PWM-capable
#define HV_MAX_PWM        255     // 9 kV at EMCO output
#define HV_DEV_MAX_PWM    180     // 6.3 kV; use during bench testing

// Gait timing
// Measured SES joint response (L0WS film, 5 cSt fluid, 9 kV):
//   Rise time (0 to 90% bend) : 12 ms
//   Fall time (90% to rest)   : 31 ms  <-- minimum phase duration floor
// Phase duration must exceed fall time to prevent sync drift.
#define RISE_TIME_MS        12
#define FALL_TIME_MS        31
#define PHASE_DURATION_MS   50    // 50 ms gives 19 ms margin above floor

// Serial
#define SERIAL_BAUD         9600

// Safety
// Use OC100HG optocouplers only. Standard types (4N35, PC817) will arc at HV.
// Install 1 MOhm bleed resistors across each joint for capacitive discharge.
// Set HV_CONTROL_PIN LOW and wait at least 100 ms before handling any circuit.

#endif
