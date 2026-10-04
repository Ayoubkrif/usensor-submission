#!/usr/bin/env python3
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "usensor-submission"


def fnv1a32(data: bytes) -> int:
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def header() -> bytes:
    base = b"USENS001" + struct.pack("<HH", 32, 1)
    return base + struct.pack("<I", fnv1a32(base))


def record(ts: int, seq: int, sensor_id: int, flags: int, x: int, y: int, z: int) -> bytes:
    base = struct.pack("<QIBBHiii", ts, seq, sensor_id, flags, 0, x, y, z)
    return base + struct.pack("<I", fnv1a32(base))


def main() -> None:
    subprocess.run(["make"], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)

    event_ts = 2_000_000_000
    trace = b"".join(
        [
            header(),
            record(event_ts - 1_000_000, 5, 2, 1, 380, 1, 2),
            record(event_ts, 2, 5, 1, 1, 0, 0),
            record(event_ts + 500_000, 6, 2, 1, 40000, 9, 9),
            record(event_ts + 20_000_000, 60, 1, 1, 60, 0, 0),
            record(event_ts - 10_000_000, 10, 3, 1, 488566000, 23522000, 35000),
            record(event_ts + 150_000_000, 7, 2, 1, 420, 3, 4),
        ]
    )

    result = subprocess.run([str(BIN)], input=trace, cwd=ROOT, check=True, capture_output=True)
    lines = [line for line in result.stdout.decode().splitlines() if line.strip()]
    assert len(lines) == 1, lines

    obj = json.loads(lines[0])
    assert obj["event_id"] == 2
    assert obj["timestamp_ns"] == event_ts
    assert obj["camera"]["frame"] == 60
    assert obj["imu"]["yaw_cd"] == 380
    assert len(obj["imu_window"]) == 1
    assert obj["imu_window"][0]["yaw_cd"] == 380
    assert obj["gps"]["lat_e7"] == 488566000


if __name__ == "__main__":
    main()
