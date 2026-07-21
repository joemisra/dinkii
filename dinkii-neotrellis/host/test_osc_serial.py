#!/usr/bin/env python3
"""End-to-end OSC-to-mext test for the private MechaTrellis commands."""

import argparse
import os
import pty
import select
import socket
import struct
import subprocess
import tempfile
import time
from pathlib import Path


SERIAL_LENGTHS = {
    0x00: 1,
    0x01: 1,
    0x05: 1,
    0x12: 1,
    0xA0: 6,
    0xA1: 4,
    0xA2: 4,
    0xA3: 2,
    0xA4: 2,
    0xA5: 6,
    0xA6: 4,
    0xA7: 2,
    0xA8: 2,
}

EXPECTED = [
    bytes((0xA0, 3, 4, 255, 64, 0)),
    bytes((0xA1, 0, 2, 255)),
    bytes((0xA2, 5, 6, 127)),
    bytes((0xA3, 128)),
    bytes((0xA4, 96)),
    bytes((0xA5, 7, 8, 12, 34, 56)),
    bytes((0xA6, 0, 128, 255)),
    bytes((0xA7, 2)),
    bytes((0xA8, 2)),
]


def osc_string(value: str) -> bytes:
    encoded = value.encode() + b"\0"
    return encoded + b"\0" * ((-len(encoded)) % 4)


def osc_message(path: str, values: list[int]) -> bytes:
    return osc_string(path) + osc_string("," + "i" * len(values)) + b"".join(
        struct.pack(">i", value) for value in values
    )


def send_osc(port: int, path: str, values: list[int]) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(osc_message(path, values), ("127.0.0.1", port))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "serialosc_device",
        nargs="?",
        default=str(Path(__file__).parent / "dist/bin/serialosc-device"),
    )
    args = parser.parse_args()
    executable = Path(args.serialosc_device).resolve()
    if not executable.is_file():
        parser.error(f"serialosc-device not found: {executable}")

    master_fd, slave_fd = pty.openpty()
    osc_port = 19171
    received = bytearray()
    packets: list[bytes] = []
    server_ready = False

    with tempfile.TemporaryDirectory(prefix="mechatrellis-osc-test-") as temp:
        temp_path = Path(temp)
        device_path = temp_path / "tty.usbmodemm421"
        device_path.symlink_to(os.ttyname(slave_fd))
        (temp_path / "m421.conf").write_text(
            "server { port = 19171 }\n"
            'application { osc_prefix = "/test" host = "127.0.0.1" '
            "port = 19172 }\n"
            "device { rotation = 0 }\n"
        )

        process = subprocess.Popen(
            [str(executable), "--config-dir", str(temp_path), str(device_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        os.close(slave_fd)

        try:
            deadline = time.monotonic() + 8
            commands_sent = False
            while time.monotonic() < deadline and len(packets) < len(EXPECTED):
                readable, _, _ = select.select([master_fd], [], [], 0.05)
                if readable:
                    received.extend(os.read(master_fd, 256))

                while received:
                    packet_length = SERIAL_LENGTHS.get(received[0])
                    if packet_length is None:
                        raise RuntimeError(
                            f"unexpected serial identifier 0x{received[0]:02X}"
                        )
                    if len(received) < packet_length:
                        break
                    packet = bytes(received[:packet_length])
                    del received[:packet_length]

                    if packet == b"\x00":
                        os.write(master_fd, bytes((0x00, 0x01, 0x04)))
                    elif packet == b"\x01":
                        device_id = b"mechatrellis".ljust(32, b"\0")
                        os.write(master_fd, b"\x01" + device_id)
                    elif packet == b"\x05":
                        os.write(master_fd, bytes((0x03, 16, 16)))
                    elif packet == b"\x12":
                        server_ready = True
                    elif packet[0] >= 0xA0:
                        packets.append(packet)

                if server_ready and not commands_sent and process.poll() is None:
                    try:
                        send_osc(osc_port, "/test/grid/led/rgb/set", [3, 4, 255, 64, 0])
                        send_osc(osc_port, "/test/grid/led/rgb/all", [-1, 2, 300])
                        send_osc(osc_port, "/test/grid/led/level8/set", [5, 6, 127])
                        send_osc(osc_port, "/test/grid/led/level8/all", [128])
                        send_osc(osc_port, "/test/grid/led/intensity8", [96])
                        send_osc(osc_port, "/test/grid/led/color/set", [7, 8, 12, 34, 56])
                        send_osc(osc_port, "/test/grid/led/color/all", [-20, 128, 999])
                        send_osc(osc_port, "/test/grid/led/color/preset/store", [2])
                        send_osc(osc_port, "/test/grid/led/color/preset/recall", [2])
                        commands_sent = True
                    except OSError:
                        pass

            if packets != EXPECTED:
                stderr = process.stderr.read() if process.poll() is not None else ""
                raise RuntimeError(
                    f"serial packets differ\nexpected: {EXPECTED!r}\n"
                    f"received: {packets!r}\nserialosc: {stderr}"
                )
        finally:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=2)
            os.close(master_fd)

    print("OSC-to-serial MechaTrellis protocol test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
