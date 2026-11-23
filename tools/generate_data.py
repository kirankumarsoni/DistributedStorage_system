#!/usr/bin/env python3
"""Stream-generate >=N MiB CSVs per node, with intentional duplicate int32 keys."""

import argparse
import os

p = argparse.ArgumentParser(
    description="Generate local CSV input files for a 1..5 node cluster."
)
p.add_argument("--nodes", type=int, default=3)
p.add_argument("--mb", type=int, default=100)
p.add_argument("--output", default="data")
p.add_argument("--duplicate-every", type=int, default=100)
p.add_argument("--shared-keys", type=int, default=100)
p.add_argument("--value-type", choices=["string", "int32"], default="string")
a = p.parse_args()
if not 1 <= a.nodes <= 5:
    raise SystemExit("--nodes must be between 1 and 5")
if a.mb < 1 or a.duplicate_every < 2 or a.shared_keys < 1:
    raise SystemExit("invalid size/duplicate settings")
os.makedirs(a.output, exist_ok=True)
target = a.mb * 1024 * 1024
for node in range(a.nodes):
    path = os.path.join(a.output, f"node{node}.csv")
    written = 0
    i = 0
    with open(path, "w", buffering=1024 * 1024, newline="") as f:
        while written < target:
            if i % a.duplicate_every == 0:
                key = i // a.duplicate_every % a.shared_keys
                value = (
                    str(100000 + key)
                    if a.value_type == "int32"
                    else f"SHARED_VALUE_{key}"
                )  # same value for duplicate key across files
            else:
                key = 100000 + node * 10000000 + i
                if key > 2147483647:
                    raise SystemExit("generated key exceeds int32 range; reduce --mb")
                value = (
                    str((node * 1000000 + i) % 2147483647)
                    if a.value_type == "int32"
                    else f"NODE_{node}_RECORD_{i}"
                )
            row = f"{key},{value}\n"
            f.write(row)
            written += len(row.encode("utf-8"))
            i += 1
    actual = os.path.getsize(path)
    print(
        f"{path}: {actual / (1024 * 1024):.2f} MiB, rows={i}, shared duplicate keys injected"
    )
