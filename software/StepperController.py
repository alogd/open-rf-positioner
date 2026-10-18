import serial
import time


class StepperController:
    def __init__(self, port, baudrate=115200, timeout=60.0, move_timeout=10.0, debug=False):
        self.ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            timeout=timeout
        )
        self.move_timeout = move_timeout
        self.debug = debug

        # Give board time to boot/reset
        time.sleep(2.0)

        # Clear any startup text
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def close(self):
        if self.ser.is_open:
            self.ser.close()

    def _log(self, msg):
        if self.debug:
            print(msg)

    def _read_nonempty_line(self, timeout=None):
        """
        Read lines until a non-empty one is received or timeout expires.
        Returns the line as string, or None on timeout.
        """
        deadline = time.time() + (timeout if timeout is not None else self.ser.timeout)

        while time.time() < deadline:
            raw = self.ser.readline()
            if not raw:
                continue

            line = raw.decode(errors="replace").strip()
            if line == "":
                continue

            self._log(f"<< {line}")
            return line

        return None

    def _send(self, cmd):
        self._log(f">> {cmd}")
        self.ser.write((cmd + "\n").encode())
        self.ser.flush()

        resp = self._read_nonempty_line(timeout=2.0)
        if resp is None:
            raise TimeoutError(f"No response to command: {cmd}")

        return resp

    def _wait_done(self):
        deadline = time.time() + self.move_timeout

        while time.time() < deadline:
            resp = self._read_nonempty_line(timeout=1.0)

            if resp is None:
                continue

            if resp.startswith("OK DONE"):
                return float(resp.split()[2])

            if resp.startswith("OK STOPPED"):
                return float(resp.split()[2])

            if resp.startswith("ERROR"):
                raise RuntimeError(resp)

            # Ignore other informational lines if they appear

        raise TimeoutError("Timed out waiting for motion completion")

    # -------- high-level CLI --------

    def help(self):
        resp = self._send("HELP")
        if resp.startswith("CMDS:"):
            return resp
        raise RuntimeError(resp)

    def get_position(self):
        resp = self._send("POS?")
        if resp.startswith("POS "):
            return resp
        raise RuntimeError(resp)

    def home(self):
        resp = self._send("HOME")
        if resp == "OK HOMED":
            return 0.0
        raise RuntimeError(resp)

    def stop(self):
        resp = self._send("STOP")
        if resp.startswith("OK STOPPED"):
            return float(resp.split()[2])
        raise RuntimeError(resp)

    def goto(self, angle):
        resp = self._send(f"GOTO {angle}")
        if resp == "OK MOVING":
            return self._wait_done()
        if resp.startswith("OK DONE"):
            return float(resp.split()[2])
        raise RuntimeError(resp)

    def move(self, delta):
        resp = self._send(f"MOVE {delta}")
        if resp == "OK MOVING":
            return self._wait_done()
        if resp.startswith("OK DONE"):
            return float(resp.split()[2])
        raise RuntimeError(resp)

    def autocenter(self):
        resp = self._send("AUTOCENTER")
        if resp.startswith("OK AUTOCENTERED"):
            return 0.0
        raise RuntimeError(resp)

    def flash_led(self):
        resp = self._send("FLASH LED")
        if resp.startswith("OK FLASHED"):
            return 0.0
        raise RuntimeError(resp)