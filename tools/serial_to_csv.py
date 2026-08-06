#!/usr/bin/env python3
"""
Capture the logger's serial output to a timestamped CSV.

The sketch prints tab-separated "volts_mV<TAB>irradiance_Wm2". This script adds
a host timestamp to each row and writes a proper CSV.

Usage:
    python serial_to_csv.py --port COM5
    python serial_to_csv.py --port /dev/ttyUSB0 --out irradiance_2026-08-06.csv

Requires: pip install pyserial
"""

import argparse
import csv
import datetime
import sys

try:
    import serial
except ImportError:
    sys.exit("pyserial is not installed. Run: pip install pyserial")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", required=True,
                    help="Serial port, e.g. COM5 or /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--out", default=None,
                    help="Output CSV path. Defaults to a timestamped filename.")
    args = ap.parse_args()

    out_path = args.out or "irradiance_{}.csv".format(
        datetime.datetime.now().strftime("%Y%m%d_%H%M%S"))

    print(f"Reading {args.port} at {args.baud}. Writing {out_path}.")
    print("Ctrl-C to stop.\n")

    with serial.Serial(args.port, args.baud, timeout=5) as ser, \
            open(out_path, "w", newline="") as f:

        writer = csv.writer(f)
        writer.writerow(["timestamp_iso", "volts_mV", "irradiance_Wm2"])
        f.flush()

        rows = 0
        try:
            while True:
                line = ser.readline().decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                parts = line.split("\t")
                if len(parts) != 2:
                    # Header line, startup messages, scanner output.
                    print(f"  [info] {line}")
                    continue

                try:
                    volts = float(parts[0])
                    irr = float(parts[1])
                except ValueError:
                    print(f"  [info] {line}")
                    continue

                ts = datetime.datetime.now().isoformat(timespec="seconds")
                writer.writerow([ts, f"{volts:.4f}", f"{irr:.2f}"])
                f.flush()

                rows += 1
                print(f"{ts}  {volts:8.4f} mV  {irr:8.2f} W/m2")

        except KeyboardInterrupt:
            print(f"\nStopped. {rows} rows written to {out_path}.")


if __name__ == "__main__":
    main()
