#!/usr/bin/env python3
\"\"\"Run CacheLab over a small cache-configuration matrix and save CSV results.\"\"\"

import argparse
import csv
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--simulator", required=True, help="Path to cache_sim executable")
    parser.add_argument("--trace", required=True, help="Path to input trace")
    parser.add_argument("--output", required=True, help="CSV output path")
    args = parser.parse_args()

    simulator = Path(args.simulator)
    trace = Path(args.trace)
    output = Path(args.output)

    if not simulator.is_file():
        parser.error(f"simulator not found: {simulator}")
    if not trace.is_file():
        parser.error(f"trace not found: {trace}")
    if not output.parent.exists():
        parser.error(f"output directory does not exist: {output.parent}")

    configurations = [
        (1024, 64, 1, "LRU"),
        (1024, 64, 2, "LRU"),
        (1024, 64, 4, "LRU"),
        (4096, 64, 1, "LRU"),
        (4096, 64, 4, "LRU"),
        (1024, 64, 2, "FIFO"),
    ]

    rows = []
    for capacity, block_size, associativity, replacement in configurations:
        command = [
            str(simulator),
            "--capacity", str(capacity),
            "--block-size", str(block_size),
            "--associativity", str(associativity),
            "--replacement", replacement,
            "--trace", str(trace),
            "--csv", str(output.parent / "_cachelab_temp_result.csv"),
        ]
        completed = subprocess.run(command, text=True, capture_output=True)
        if completed.returncode != 0:
            print(completed.stderr, file=sys.stderr)
            return completed.returncode

        # The simulator writes a single data row after the CSV header.
        temp_csv = output.parent / "_cachelab_temp_result.csv"
        with temp_csv.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            row = next(reader)
        row["trace"] = str(trace)
        rows.append(row)
        temp_csv.unlink(missing_ok=True)

    fieldnames = ["trace"] + list(rows[0].keys() - {"trace"}) if rows else ["trace"]
    # Preserve stable, readable column order rather than set order.
    if rows:
        fieldnames = [
            "trace", "capacity_bytes", "block_size_bytes", "associativity",
            "replacement", "accesses", "reads", "writes", "hits", "misses",
            "hit_rate_percent", "miss_rate_percent",
        ]

    with output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Saved {len(rows)} configuration results to {output}")
    print(f"{'Capacity':>8} {'Block':>6} {'Ways':>5} {'Policy':>7} "
          f"{'Hits':>8} {'Misses':>8} {'Hit rate':>10}")
    for row in rows:
        print(f"{row['capacity_bytes']:>8} {row['block_size_bytes']:>6} "
              f"{row['associativity']:>5} {row['replacement']:>7} "
              f"{row['hits']:>8} {row['misses']:>8} "
              f"{float(row['hit_rate_percent']):>9.2f}%")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
