
"""
basic_sweep.py - minimal example of driving the open-rf-positioner from Python.

Finds boresight with AUTOCENTER, then steps through a range of angles. At each
stop is where you would trigger your SDR acquisition, tagging the capture with
the angle the firmware reports back.

Edit the settings below, then run:  python basic_sweep.py

Requires: pyserial, and StepperController.py on the Python path
"""

import time

from StepperController import StepperController

PORT     = "/dev/ttyACM0"   # "/dev/ttyACM0" (Linux) or "COM5" (Windows)
START    = -60.0            # sweep start angle (deg)
STOP     = 60.0             # sweep stop angle (deg)
STEP     = 10.0             # step size (deg)
SETTLE   = 0.5              # pause after each move before acquiring (s)


def main():
    stage = StepperController(port=PORT)
    try:
        stage.autocenter()                     # define mechanical center as 0 deg

        angle = START
        while angle <= STOP:
            reported = stage.goto(angle)       # blocks until the move finishes
            time.sleep(SETTLE)                 # let the structure settle

            # >>> trigger your SDR acquisition here, tagged with `reported` <<<
            print(f"commanded = {angle:+6.2f} deg   reported = {reported:+6.2f} deg")

            angle += STEP

        stage.goto(0.0)                        # return to center
    finally:
        stage.close()


if __name__ == "__main__":
    main()