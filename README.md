# usensor-submission

## Build

```sh
make
```

## Run

```sh
./usensor --seed 42 --duration 300s | ./usensor-submission
```

## Approach

The program reads exactly 16 bytes for the header, validates magic/version/record-size and verifies the FNV-1a checksum. It then reads 32-byte records in a loop and rejects checksum failures, invalid-flag records, and truncated final records.

Synchronization uses bounded in-memory buffers per sensor (`camera`, `imu`, `gps`) plus a pending button queue. Since records may arrive slightly out of timestamp order, button events are emitted once the stream has advanced at least `event_ts + 100ms + 2ms`, ensuring complete IMU window coverage before output. At EOF, remaining pending events are flushed.

Selection rules implemented:

- camera: nearest sample within ±50 ms (ties: earlier timestamp, then lower sequence)
- imu: nearest usable sample within ±20 ms (`yaw` must be in `[0, 35999]`)
- gps: latest sample at or before event timestamp and no older than 1 s
- imu_window: all usable IMU samples within ±100 ms, sorted by timestamp then sequence

Memory stays bounded by pruning old sensor samples based on the latest observed timestamp and earliest pending button event.

## Focused test

```sh
python3 tests/test_invalid_imu.py
```

This test builds a tiny binary stream with one button event where the nearest IMU sample is invalid (`yaw` out of range), ensuring both `imu` and `imu_window` ignore that sample while camera and GPS selection still work.
