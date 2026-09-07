/*
 * swarm_voltage_control.ino
 * SWARM Crab Robot
 * HV amplifier voltage control via PWM and RC filter.
 *
 * Author : Daniel Huang
 * Program: UCLA ECE Fast-Track B.S., Class of 2030
 *
 * The Arduino does not have a built-in DAC on most boards, so a PWM
 * output is smoothed by an external RC low-pass filter to produce an
 * analog control voltage for the EMCO G-series HV amplifier.
 *
 * RC filter:
 *   R = 10 kOhm, C = 10 uF
 *   Time constant tau = R * C = 100 ms
 *   Cutoff frequency  = 1 / (2 * pi * tau) = approximately 1.6 Hz
 *   Settling time for full-scale step: approximately 500 ms
 *
 * EMCO G-series transfer:
 *   Input 0-5V maps to output 0-9 kV
 *   Gain approximately 1800 V/V
 *
 * SES joint force follows the Maxwell stress equation:
 *   F = (e0 * er * A * V^2) / (2 * d^2)
 * Force scales with V squared, so joint bend angle is nonlinear
 * with respect to voltage. Refer to the angle-voltage curves in
 * the project documentation to calibrate for a specific joint geometry.
 *
 * Requires: config.h
 */

#include "../config.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(HV_CONTROL_PIN, OUTPUT);
  analogWrite(HV_CONTROL_PIN, 0);
  Serial.println("Voltage control ready.");
  Serial.println("Commands: V:<0.0-1.0> | RAMP:<0.0-1.0> | OFF");
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("V:")) {
    float frac = constrain(cmd.substring(2).toFloat(), 0.0, 1.0);
    setVoltage(frac);

  } else if (cmd.startsWith("RAMP:")) {
    float frac = constrain(cmd.substring(5).toFloat(), 0.0, 1.0);
    rampVoltage(frac, 8);  // 8 ms per PWM step

  } else if (cmd == "OFF") {
    setVoltage(0.0);
    Serial.println("HV off.");

  } else if (cmd.length() > 0) {
    Serial.println("Commands: V:<0.0-1.0> | RAMP:<0.0-1.0> | OFF");
  }
}

/*
 * setVoltage(fraction)
 * Sets the EMCO output as a fraction of the development ceiling.
 * Swap HV_DEV_MAX_PWM for HV_MAX_PWM when ready for full 9 kV operation.
 */
void setVoltage(float fraction) {
  int pwm = (int)(fraction * HV_DEV_MAX_PWM);
  analogWrite(HV_CONTROL_PIN, pwm);
  float kv = fraction * 6.3;
  Serial.print("PWM "); Serial.print(pwm);
  Serial.print("  (~"); Serial.print(kv, 1); Serial.println(" kV)");
}

/*
 * rampVoltage(targetFraction, stepMs)
 * Smoothly increases voltage to the target level.
 * Avoids sudden steps that stress the dielectric film.
 * Recommended for first-time joint testing on a new build.
 */
void rampVoltage(float targetFraction, int stepMs) {
  int targetPWM  = (int)(constrain(targetFraction, 0.0, 1.0) * HV_DEV_MAX_PWM);
  int currentPWM = 0;
  while (currentPWM < targetPWM) {
    currentPWM++;
    analogWrite(HV_CONTROL_PIN, currentPWM);
    delay(stepMs);
  }
  Serial.print("Ramp done at PWM "); Serial.println(targetPWM);
}
