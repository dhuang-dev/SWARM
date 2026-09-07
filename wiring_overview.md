# SWARM Wiring Overview

## Arduino pin assignments

All assignments are defined in `config.h`. Default layout for Arduino Mega 2560:

| Joint index | Leg | Joint | Arduino pin |
|---|---|---|---|
| 0  | 0 | Hip  | D2  |
| 1  | 0 | Knee | D3  |
| 2  | 1 | Hip  | D4  |
| 3  | 1 | Knee | D5  |
| 4  | 2 | Hip  | D6  |
| 5  | 2 | Knee | D7  |
| 6  | 3 | Hip  | D8  |
| 7  | 3 | Knee | D9  |
| 8  | 4 | Hip  | D10 |
| 9  | 4 | Knee | D11 |
| 10 | 5 | Hip  | D12 |
| 11 | 5 | Knee | D13 |
| 12 | 6 | Hip  | A0  |
| 13 | 6 | Knee | A1  |
| 14 | 7 | Hip  | A2  |
| 15 | 7 | Knee | A3  |

HV control (PWM out to RC filter): D9

## OC100HG optocoupler wiring (per joint)

```
Arduino digital pin  -->  OC100HG LED anode  (pin 1)
GND                  -->  OC100HG LED cathode (pin 2)
HV amplifier output  -->  OC100HG collector   (pin 5)
SES joint electrode  -->  OC100HG emitter     (pin 4)
1 MOhm resistor between collector and emitter (bleed path)
```

## RC low-pass filter (HV control)

```
Arduino D9 (PWM)
    |
   [10 kOhm]
    |----+---- to EMCO G-series V_in
    |    |
   [10 uF]
    |
   GND
```

Time constant: 100 ms. Settling time for full-scale step: approximately 500 ms.

## EMCO G-series connections

| EMCO pin | Connect to |
|---|---|
| V_in | RC filter output |
| GND  | Arduino GND and chassis GND |
| HV+  | OC100HG collector rail (shared across all 16 optocouplers) |
| HV-  | SES joint shared ground electrode |

## Companion computer (RPi/Jetson) to Arduino

Connect via USB-A to USB-B (standard Arduino cable).
Port is typically `/dev/ttyUSB0` or `/dev/ttyACM0` on Linux.
Baud rate: 9600 (set in `config.h`).

## Safety checklist before powering on

- [ ] All 16 bleed resistors (1 MOhm) installed across joint electrodes
- [ ] No exposed HV traces or wires within reach during operation
- [ ] EMCO output connected to OC100HG collectors only, not directly to joints
- [ ] Arduino GND and EMCO GND are common
- [ ] HV_CONTROL_PIN set to LOW in firmware before any other setup runs
- [ ] HV_DEV_MAX_PWM used as ceiling during initial testing
