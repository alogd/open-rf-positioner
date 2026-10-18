# Software

Host-side Python for driving the positioning base over USB serial. This folder
contains the control driver and a minimal usage example — nothing else.


The driver speaks the firmware's line-oriented serial protocol (115200 baud) and
exposes the stage as angle-level method calls. See [`../firmware/`](../firmware/)
for the underlying command set.

## Requirements

- Python 3.8+
- [`pyserial`](https://pypi.org/project/pyserial/)

```bash
pip install pyserial
```

## Quick start

Run from this `software/` directory so the example can import the driver:

```bash
python basic_sweep.py
```

Edit the settings at the top of `basic_sweep.py` first — serial port, sweep
`START`/`STOP`/`STEP`, and the settle time:

```python
PORT   = "/dev/ttyACM0"   # "/dev/ttyACM0" (Linux) or "COM5" (Windows)
START  = -60.0
STOP   = 60.0
STEP   = 10.0
SETTLE = 0.5
```

## Using the driver directly

```python
from StepperController import StepperController

stage = StepperController(port="/dev/ttyACM0")   # waits ~2 s for the board to boot
try:
    stage.autocenter()            # find mechanical boresight → 0°
    stage.goto(30)                # absolute move to +30°, returns the final angle
    stage.move(-5)                # relative move, returns the final angle
    print(stage.get_position())   # e.g. "POS 25.00 steps=2222"
    stage.home()                  # set the current position as 0°
finally:
    stage.close()
```

### Constructor

`StepperController(port, baudrate=115200, timeout=60.0, move_timeout=10.0, debug=False)`

- `port` — serial device, e.g. `/dev/ttyACM0` or `COM5`.
- `move_timeout` — seconds to wait for a move to finish before raising `TimeoutError`.
- `debug=True` — echo the raw serial traffic (`>>` sent, `<<` received).

### Methods

| Method | Returns | Description |
|---|---|---|
| `goto(angle)` | `float` | Move to an absolute angle; blocks until the move completes. |
| `move(delta)` | `float` | Move relative to the current angle; blocks until complete. |
| `get_position()` | `str` | Raw `POS <ang> steps=<n>` reply. |
| `home()` | `float` | Set the current position as 0°. |
| `autocenter()` | `float` | Drive to both limits and stop at the midpoint (boresight). |
| `stop()` | `float` | Abort the current move; returns the angle reached. |
| `flash_led()` | `float` | Blink the onboard LED to identify the board. |
| `help()` | `str` | Firmware command list. |
| `close()` | — | Close the serial port. |

Any `ERROR …` reply from the firmware (e.g. out-of-range or busy) is raised as a
`RuntimeError`; a missing reply or an overrun `move_timeout` raises `TimeoutError`.
Always `close()` when done (the example uses a `try/finally`).

## Notes

- The travel limits (±65°) and all motion safety checks are enforced by the
  firmware, not here.
- Opening the port resets the board, so the constructor pauses ~2 s before the
  stage is ready to take commands.