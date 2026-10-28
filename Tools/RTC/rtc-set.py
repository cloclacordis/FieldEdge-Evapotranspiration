#!/usr/bin/env python3
"""Set and verify the STM32 RTC from the host PC's UTC clock."""

import argparse
from datetime import datetime, timezone
import sys
import time

try:
    import serial
    from serial import SerialException
except ImportError:
    print(
        "pyserial is missing. Install it with: "
        "python -m pip install -r Tools/RTC/requirements.txt",
        file=sys.stderr,
    )
    raise SystemExit(2)

BAUDRATE = 115200
RESPONSE_TIMEOUT_S = 10.0
QUERY_INTERVAL_S = 0.75
MAX_CLOCK_DIFFERENCE_S = 3.0


def send_command(port, command):
    port.write((command + "\r\n").encode("ascii"))
    port.flush()


def read_line(port):
    raw = port.readline()
    if not raw:
        return None

    line = raw.decode("ascii", errors="replace").rstrip("\r\n")
    print(line, flush=True)
    return line


def wait_for_time_response(port):
    """Query repeatedly."""
    deadline = time.monotonic() + RESPONSE_TIMEOUT_S
    next_query = 0.0
    state = None
    utc_text = None

    while time.monotonic() < deadline:
        now = time.monotonic()
        if now >= next_query:
            send_command(port, "TIME")
            next_query = now + QUERY_INTERVAL_S

        line = read_line(port)
        if line is None:
            continue

        if line.startswith("RTC state: "):
            state = line.split(":", 1)[1].strip()
        elif line.startswith("UTC time unavailable:"):
            return state, None
        elif line.startswith("UTC: "):
            utc_text = line[len("UTC: "):].strip()
        elif utc_text is not None and line.startswith(
            ("Station date:", "Station date unavailable:")
        ):
            return state, utc_text

    raise RuntimeError(
        "No complete TIME response. Check the port, 115200 baud, "
        "and that the firmware is running."
    )


def parse_utc(text):
    value = datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ")
    return value.replace(tzinfo=timezone.utc)


def set_time_from_host(port):
    host_time = datetime.now(timezone.utc).replace(microsecond=0)
    command = "SET-UTC " + host_time.strftime("%Y-%m-%dT%H:%M:%S") + "Z"

    print("\nSetting the RTC from host UTC:", command, flush=True)
    send_command(port, command)

    deadline = time.monotonic() + RESPONSE_TIMEOUT_S
    status = None
    state = None
    utc_text = None
    station_line_seen = False

    while time.monotonic() < deadline:
        line = read_line(port)
        if line is None:
            continue

        if line.startswith("SET-UTC:"):
            status = line.split(":", 1)[1].strip()
            if status != "STATUS_OK":
                raise RuntimeError("MCU rejected SET-UTC: " + status)
        elif line.startswith("RTC state: "):
            state = line.split(":", 1)[1].strip()
        elif line.startswith("UTC time unavailable:"):
            raise RuntimeError("The MCU could not read UTC after setting the RTC.")
        elif line.startswith("UTC: "):
            utc_text = line[len("UTC: "):].strip()
        elif line.startswith(("Station date:", "Station date unavailable:")):
            station_line_seen = True

        if status == "STATUS_OK" and state is not None and utc_text is not None and station_line_seen:
            break

    if status != "STATUS_OK":
        raise RuntimeError("Timed out waiting for SET-UTC acknowledgement.")
    if state != "RTC_STATE_TRUSTED":
        raise RuntimeError("RTC did not become trusted; state: " + str(state))
    if utc_text is None:
        raise RuntimeError("No UTC read-back received from the MCU.")

    device_time = parse_utc(utc_text)
    difference = abs((datetime.now(timezone.utc) - device_time).total_seconds())
    if difference > MAX_CLOCK_DIFFERENCE_S:
        raise RuntimeError(
            "RTC read-back differs from host UTC by %.1f s. "
            "Check that the host clock is synchronized." % difference
        )

    print("\nRTC verified: %s (host difference %.1f s)." % (utc_text, difference))


def main():
    parser = argparse.ArgumentParser(
        description="Set and verify an STM32F446RE RTC using the host UTC clock."
    )
    parser.add_argument("--port", required=True, help="COM4 or /dev/ttyACM0")
    args = parser.parse_args()

    print("The host clock must be correct and synchronized to UTC.")
    print("This operation sets the MCU clock.\n")

    port = serial.Serial(
        port=None, baudrate=BAUDRATE, timeout=0.1, write_timeout=1.0
    )
    port.port = args.port
    port.dtr = False
    port.rts = False

    try:
        port.open()
        state, _ = wait_for_time_response(port)
        if state == "RTC_STATE_CLOCK_FAULT":
            raise RuntimeError(
                "RTC hardware clock fault. Check the LSE crystal and RTC initialization."
            )
        set_time_from_host(port)
        return 0
    except (SerialException, OSError, RuntimeError, ValueError) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nCancelled.", file=sys.stderr)
        return 130
    finally:
        if port.is_open:
            port.close()


if __name__ == "__main__":
    sys.exit(main())
