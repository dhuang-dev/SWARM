# SWARM Crab Robot — Firmware

**Spider-Inspired Electrohydraulic Soft-Actuated (SES) Joint System**
Author: Daniel Huang | UCLA ECE Fast-Track B.S., Class of 2030

---

## What this is

Firmware for a crab-morphology disaster-response robot that uses SES electrohydraulic joints instead of servo motors. Each joint is a sealed pouch of liquid dielectric oil with screen-printed carbon electrodes. High voltage causes the electrodes to zip together (Maxwell stress), pressurizing the oil and bending a rigid-compliant hinge. No motors, no gears, near-zero hold power.

The robot has 8 legs, 2 joints per leg (16 joints total), each switched by an OC100HG optocoupler driven from an Arduino digital pin.

---

## Repository structure

```
SWARM_firmware/
  config.h                              Shared pin map, timing constants, voltage limits
  swarm_gait_controller/
    swarm_gait_controller.ino           4-phase tetrapod gait state machine
  swarm_voltage_control/
    swarm_voltage_control.ino           PWM-based HV amplifier control
  swarm_serial_interface/
    swarm_serial_interface.ino          Serial command bridge (Arduino side)
  swarm_companion_python/
    swarm_companion.py                  Companion computer bridge (RPi/Jetson side)
  docs/
    wiring_overview.md                  Signal chain and safety notes
```

---

## Hardware required

| Component | Part | Notes |
|---|---|---|
| Microcontroller | Arduino Mega 2560 | Enough PWM and digital pins for 16 joints |
| HV amplifier | EMCO G50 or G100 | 0-5V input, 0-5kV or 0-10kV output |
| HV switches | OC100HG optocouplers | One per joint; standard optocouplers will arc |
| Bleed resistors | 1 MOhm, 1/4W | One per joint, in parallel for safe discharge |
| RC filter | 10 kOhm + 10 uF | Smooths PWM to analog for EMCO input |
| Companion computer | Raspberry Pi 4 or Jetson Nano | Runs SLAM and AI; connects via USB serial |

---

## Signal chain

```
Arduino PWM pin
    |
    v
RC low-pass filter (10 kOhm + 10 uF, tau = 100 ms)
    |
    v
EMCO G-series HV amplifier (0-5V in, 0-9 kV out)
    |
    +-- OC100HG optocoupler channel 0  -->  SES Joint 0  (Leg 0, Hip)
    +-- OC100HG optocoupler channel 1  -->  SES Joint 1  (Leg 0, Knee)
    |   ... (16 channels total)
    +-- OC100HG optocoupler channel 15 -->  SES Joint 15 (Leg 7, Knee)

Each joint: 1 MOhm bleed resistor in parallel for capacitive discharge.
```

---

## SES joint timing constraints

These are measured values from Kellaris et al. (2021) for L0WS film, 5 cSt fluid, 9 kV:

| Metric | Value |
|---|---|
| Rise time (0 to 90% bend) | 12 ms |
| Fall time (90% to resting) | 31 ms |
| Roll-off bandwidth | 24 Hz |
| Specific torque | 21.2 N·m/kg |
| Hold power | under 1 mW |

**The 31 ms fall time is a hard physical floor for gait phase duration.** Setting `PHASE_DURATION_MS` below this causes joints to stack activation cycles and drift out of sync. The default is 50 ms, giving 19 ms of margin.

---

## Getting started

1. Wire the hardware per `docs/wiring_overview.md`
2. Open `config.h` and verify pin assignments match your build
3. Flash `swarm_gait_controller.ino` to the Arduino
4. Open Serial Monitor at 9600 baud
5. Send `G:forward` to start the gait, `G:stop` to halt
6. For companion computer control, run `swarm_companion.py --demo`

---

## Serial command reference

| Command | Action |
|---|---|
| `G:forward` | Start alternating tetrapod gait |
| `G:stop` | Halt all joints immediately |
| `V:<0.0-1.0>` | Set HV fraction (e.g. V:0.75) |
| `J:<leg>:<joint>` | Fire one joint once (leg 0-7, joint 0=hip 1=knee) |
| `STATUS` | Print current phase and voltage |
| `PING` | Responds PONG (connection check) |

---

## Safety

- **Never touch the circuit while powered.** SES joints store charge like capacitors and hold it after disconnect.
- Always wait at least 100 ms after cutting power before handling any joint or wire.
- Bleed resistors (1 MOhm per joint) are mandatory, not optional.
- Use `HV_DEV_MAX_PWM` (approximately 6.3 kV ceiling) during bench testing. Only switch to `HV_MAX_PWM` for full 9 kV when the chassis is fully assembled and no wiring is exposed.

---

## Reference

Kellaris, N. et al. (2021). Spider-Inspired Electrohydraulic Actuators for Fast, Soft-Actuated Joints. *Advanced Science*, 8(14), 2100916. DOI: 10.1002/advs.202100916
