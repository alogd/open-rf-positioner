# Firmware

Motor-control firmware for the **ESP32-H2** that drives the A4988/NEMA 17 and exposes the serial command interface to the host.

Source lives in [`esp32-h2/`](esp32-h2/).

## Behavior

- **Line-oriented serial** over USB at **115200 baud** (command set below).
- **Open-loop with a drift-free reference**: a signed **step count** is the true position; angle is derived as `step_count × 0.01125°` (0.09° full-step at the arm ÷ 1/8 microstepping). On a completed move the reported angle snaps to the requested target for a clean readout while the step count stays exact, so repeated relative moves don't accumulate rounding error.
- **Trapezoidal velocity profile** on large moves (parabolic ramp 800 µs → 300 µs per step over the first/last 300 steps, constant in the middle; short moves use a triangular profile) so no steps are lost and commanded vs. actual angle stay in agreement.
- **Limit switches at ±65°** act as safety stops and absolute references: on contact, an interrupt sets the stored angle/step count to the known extreme, re-synchronizing the open-loop position. Commands beyond ±65° are rejected.
- **`AUTOCENTER`** drives to both limits, counts the steps between them, and returns to the midpoint — defining 0° as the mechanical center without manual alignment.
- **Retained step count** survives soft resets, so the stage keeps its reference without re-homing after a restart.
- **Onboard RGB LED** mirrors limit/centering states for a quick visual check during setup.

## Command set

| Command | Reply | Description |
|---|---|---|
| `GOTO <deg>` | `OK MOVING` → `OK DONE <ang>` | Move to an absolute angle (rejected outside ±65°) |
| `MOVE <deg>` | `OK MOVING` → `OK DONE <ang>` | Move relative to the current angle |
| `POS?` | `POS <ang> steps=<n>` | Report current angle and step count |
| `HOME` | `OK HOMED` | Set the current position as 0° |
| `AUTOCENTER` | `OK AUTOCENTERED` | Drive to both limits, stop at the midpoint (boresight) |
| `STOP` | `OK STOPPED <ang>` | Abort the current move |
| `FLASH LED` | `OK FLASHED` | Blink the onboard LED for identification |
| `HELP` | `CMDS: ...` | List available commands |

Invalid input returns `ERROR OUT_OF_RANGE`, `ERROR BUSY`, or `ERROR UNKNOWN_CMD`.


## Tunable parameters

All of these live in the `CONFIG` block at the top of the firmware source (a few derived constants follow it). Defaults are the as-built values.

### Pins (ESP32-H2)

![Wiring diagram](../docs/images/wiring_diagram.png)

| Constant | Default | Description |
|---|---|---|
| `STEP_PIN` | `11` | A4988 STEP output |
| `DIR_PIN` | `12` | A4988 DIR output |
| `limitSwitch_maxAngle` | `5` | GPIO for the +65° limit switch (INPUT_PULLUP, RISING interrupt) |
| `limitSwitch_minAngle` | `13` | GPIO for the −65° limit switch (INPUT_PULLUP, RISING interrupt) |
| RGB LED | `8` | Onboard status LED (hard-coded in the `rgbLedWrite(8, …)` calls) |

### Kinematics

| Constant | Default | Description |
|---|---|---|
| `STEP_DIVISION` | `8.0` | Microstepping divisor. **Must match the A4988 MS1/MS2/MS3 jumpers** — if the wiring and this value disagree, every commanded angle is scaled wrong. |
| `STEP_ANGLE` | `0.09 / STEP_DIVISION` = `0.01125°` | Degrees per microstep (derived). The `0.09` is the full-step angle *at the arm* = motor step (1.8°) ÷ gear reduction (20:1). **Change the `0.09` if you change the motor or the gear ratio.** |

### Travel limits

| Constant | Default | Description |
|---|---|---|
| `MIN_ANGLE` / `MAX_ANGLE` | `−65.0` / `65.0` | Software travel bounds. `GOTO`/`MOVE` outside this return `ERROR OUT_OF_RANGE`, and on a limit-switch hit the position is re-referenced to these values. Keep them consistent with where the switches are physically mounted. |
| `debounceAngle` | `3.0` | Degrees the stage must back off a limit before that switch's latch is cleared — prevents immediate re-triggering while still on the switch. |

### Speed and acceleration

| Constant | Default | Description |
|---|---|---|
| `MIN_DELAY_US` | `300.0` | Shortest per-pulse delay → **top speed** (ramp plateau). Lower = faster but risks lost steps. |
| `MAX_DELAY_US` | `800.0` | Longest per-pulse delay → **start/end speed** of the ramp. |
| `ACC_PROFILE_STEPS` | `300.0` | Length of the accel (and decel) ramp in steps. Longer = gentler ramp. Moves longer than `2 × ACC_PROFILE_STEPS` get a full accel–plateau–decel (trapezoidal) profile; shorter moves use a triangular profile with no plateau. |
| `STEP_DELAY_US` | `400` | Constant per-pulse delay used during `AUTOCENTER` (no ramp). |

> Each step toggles the pin with the delay applied twice (HIGH then LOW), so the per-step period is `2 × delay` µs — e.g. the `300 µs` plateau is ≈ 1667 steps/s ≈ 18.8°/s at the default resolution.

### Other

| Constant | Default | Description |
|---|---|---|
| `LED_FLASH_DELAY` | `150` | Blink interval (ms) for the `FLASH LED` identify routine. |
| `DEBUG` | *(commented out)* | Uncomment `#define DEBUG` to enable limit-switch debounce debugging and the LED debug blink. |

### Derived (don't edit directly)

`a_p` and `c_p` are computed from `MAX_DELAY_US`, `MIN_DELAY_US`, and `ACC_PROFILE_STEPS` to shape the parabolic ramp — they recompute automatically when you change those three, so there's no need to touch them.

