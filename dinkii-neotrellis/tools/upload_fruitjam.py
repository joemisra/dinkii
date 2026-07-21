#!/usr/bin/env python3
"""Build and upload Fruit Jam firmware through the private Picoboot command."""

from __future__ import annotations

import argparse
import errno
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys
import termios
import time


PROJECT_DIR = Path(__file__).resolve().parent.parent
FIRMWARE = PROJECT_DIR / ".pio/build/fruitjam/firmware.elf"
CLEAR_LEDS_PACKET = b"\x12"
BOOTLOADER_PACKET = b"\xF0BOOT"
CUSTOM_SERIALOSC_LABEL = "coffee.dsp.mechatrellis-serialosc"
CUSTOM_SERIALOSC_PLIST = (
    Path.home() / "Library/LaunchAgents" / f"{CUSTOM_SERIALOSC_LABEL}.plist"
)


def run(command: list[str], *, check: bool = True, quiet: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=PROJECT_DIR,
        check=check,
        text=True,
        stdout=subprocess.DEVNULL if quiet else None,
        stderr=subprocess.DEVNULL if quiet else None,
    )


def serial_ports() -> list[str]:
    return sorted(glob.glob("/dev/cu.usbmodem*"))


def open_serial_port(port: str, timeout: float = 5.0) -> int:
    deadline = time.monotonic() + timeout
    while True:
        try:
            return os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        except OSError as error:
            if error.errno not in (errno.EBUSY, errno.EACCES) or time.monotonic() >= deadline:
                raise
            time.sleep(0.1)


def send_bootloader_command(port: str) -> None:
    fd = open_serial_port(port)
    try:
        settings = termios.tcgetattr(fd)
        settings[0] = 0
        settings[1] = 0
        settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        settings[3] = 0
        settings[4] = termios.B115200
        settings[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, settings)
        time.sleep(0.1)
        # NeoPixels retain their state while the RP2350 is in Picoboot. Clear
        # the physical grid first so a bright frame cannot brown out reboot.
        os.write(fd, CLEAR_LEDS_PACKET)
        termios.tcdrain(fd)
        time.sleep(0.25)
        os.write(fd, BOOTLOADER_PACKET)
        termios.tcdrain(fd)
    finally:
        os.close(fd)


def find_picotool() -> str:
    candidates = [
        shutil.which("picotool"),
        str(
            Path.home()
            / ".platformio/packages/tool-picotool-rp2040-earlephilhower/picotool"
        ),
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    raise RuntimeError("picotool was not found; install the Fruit Jam PlatformIO environment first")


def upload(picotool: str, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = run(
            [picotool, "load", "-v", "-x", str(FIRMWARE)],
            check=False,
            quiet=True,
        )
        if result.returncode == 0:
            return
        time.sleep(0.2)
    raise RuntimeError("Picoboot did not become available before the upload timeout")


def custom_serialosc_target() -> str:
    return f"gui/{os.getuid()}/{CUSTOM_SERIALOSC_LABEL}"


def custom_serialosc_is_loaded() -> bool:
    return (
        run(
            ["launchctl", "print", custom_serialosc_target()],
            check=False,
            quiet=True,
        ).returncode
        == 0
    )


def stop_custom_serialosc() -> None:
    run(["launchctl", "bootout", custom_serialosc_target()])


def start_custom_serialosc() -> None:
    if not CUSTOM_SERIALOSC_PLIST.is_file():
        raise RuntimeError(
            f"custom serialosc LaunchAgent is missing: {CUSTOM_SERIALOSC_PLIST}"
        )
    run(
        [
            "launchctl",
            "bootstrap",
            f"gui/{os.getuid()}",
            str(CUSTOM_SERIALOSC_PLIST),
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="USB serial port; defaults to the only /dev/cu.usbmodem device")
    parser.add_argument("--no-build", action="store_true", help="upload the existing fruitjam build")
    parser.add_argument(
        "--manual",
        action="store_true",
        help="wait for a manually entered bootloader instead of sending the private command",
    )
    parser.add_argument("--timeout", type=float, default=15.0)
    args = parser.parse_args()

    pio = shutil.which("pio")
    if not pio:
        raise RuntimeError("PlatformIO CLI (pio) was not found")

    if not args.no_build:
        run([pio, "run", "-e", "fruitjam"])
    if not FIRMWARE.is_file():
        raise RuntimeError(f"firmware image does not exist: {FIRMWARE}")

    brew = shutil.which("brew")
    serialosc_stopped = False
    custom_serialosc_stopped = False
    try:
        if custom_serialosc_is_loaded():
            stop_custom_serialosc()
            custom_serialosc_stopped = True
        elif brew:
            run([brew, "services", "stop", "serialosc"], check=False)
            serialosc_stopped = True

        if args.manual:
            print("Waiting for a manually entered RP2350 bootloader...")
        else:
            ports = [args.port] if args.port else serial_ports()
            if len(ports) != 1:
                raise RuntimeError(
                    "expected one USB modem port; pass --port explicitly (found: "
                    + ", ".join(ports)
                    + ")"
                )
            print(f"Requesting Picoboot-only mode through {ports[0]}...")
            send_bootloader_command(ports[0])

        upload(find_picotool(), args.timeout)
        print("Fruit Jam firmware uploaded successfully.")
        return 0
    finally:
        if custom_serialosc_stopped:
            start_custom_serialosc()
        elif serialosc_stopped:
            run([brew, "services", "start", "serialosc"], check=False)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
