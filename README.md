# Distributed In-Memory Key-Value Storage

A Linux-only C++17 interview project that demonstrates a small distributed
in-memory key-value store using independent node processes and TCP sockets.

## Features

- Configurable cluster of 1–5 nodes.
- Configurable `string` or `int32` keys and values.
- Each node reads its own CSV input file.
- Records are routed to exactly one owner node using deterministic partitioning.
- Nodes communicate using POSIX TCP sockets.
- Bounded batches are used when transferring records.
- Duplicate keys use **last-write-wins** behavior.
- The ownership verifier checks for misplaced keys and duplicate placements.

## Requirements

- Linux
- C++17 compiler
- CMake 3.12 
- Make
- Python 3 (only for generating test data)

## Build

From the project directory:

```bash
./build_run.sh
```

This creates the executables in `build/`.

## Generate input data

The default command creates three CSV files, each at least 100 MiB:

```bash
python3 tools/generate_data.py --nodes 3 --mb 100
```

The generated files :

```text
data/node0.csv
data/node1.csv
data/node2.csv
```

The number of nodes must match `nodes` in `config.conf`.

## Run the cluster

Start the configured node processes:

```bash
./cluster_start.sh
```

Each node starts its TCP server and loads its corresponding `data/nodeN.csv`.
All node logs are saved in `logs/` as `node<ID>.log` (for example,
`logs/node0.log`). Wait until each log contains a `[LOCAL-LOAD]` message:

```text
logs/node0.log
logs/node1.log
logs/node2.log
```

## Verify ownership

After all nodes finish loading, run:

```bash
./cluster_verify.sh
```

A successful run ends with:

```text
OWNERSHIP VERIFICATION: PASS
```

The verifier reports records received, unique keys, misplaced keys, and duplicate
placements for each node.

## Stop the cluster

Stop the cluster gracefully:

```bash
./cluster_stop.sh
```
## How ownership and routing work

Each record's key is mapped to exactly one node. The ownership calculation is
implemented in `src/Distributor.cpp`: `int32` keys are converted to unsigned
32-bit values, while string keys use stable FNV-1a hashing, in both cases the
result is reduced modulo the configured node count.

`src/DataLoader.cpp` reads and validates each CSV row, asks the distributor for
the key's owner, and adds the record to a separate buffer for that node. When a
buffer reaches the configured batch size, it is flushed. Records owned by the
current node are stored locally. records owned by another node are sent to it
over TCP by `src/TcpTransport.cpp`. Any remaining partial batches are flushed
when the input file has been read. The transport waits for an acknowledgement
and closes the connection after each batch.

Per-node logs show batch and TCP connection activity. The ownership verifier
checks that records ended up on their calculated owner nodes.
