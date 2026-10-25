# CacheLab: Configurable CPU Cache Simulator

CacheLab is a small C++17 project that simulates a **single-level CPU cache** from a text-based memory-access trace. It supports direct-mapped and set-associative caches, LRU/FIFO replacement, read/write accesses, statistics, tests, and CSV experiments.

> **Scope:** This is an educational cache model. It is not a cycle-accurate CPU simulator and does not model any particular commercial processor.

## Features

- Direct-mapped and set-associative caches
- Configurable capacity, block size, and associativity
- LRU and FIFO replacement policies
- Read (`R`) and write (`W`) accesses
- 64-bit memory addresses (hexadecimal or decimal)
- Hit/miss counters and rates
- Deterministic sample traces and tests
- CSV output and a Python configuration-comparison script

## Requirements

- C++17 compiler (GCC 8+, Clang 7+, or compatible)
- CMake 3.16+
- Python 3 for `scripts/compare_configs.py` (optional)

## Build

```bash
cmake -S . -B build
cmake --build build
```

This builds:
- `build/cache_sim` — simulator
- `build/cache_tests` — test executable

## Run a simulation

```bash
./build/cache_sim \
  --capacity 4096 \
  --block-size 64 \
  --associativity 4 \
  --replacement LRU \
  --trace traces/sequential.trace
```

`--associativity 1` gives a direct-mapped cache. Cache capacity, block size, and associativity must be positive, and capacity must be divisible by `block-size * associativity`.

Use `--csv results/run.csv` to write a one-row CSV result. The output directory must already exist.

## Trace format

Each non-empty line contains an operation and an address, separated by whitespace or a comma. Lines beginning with `#` are comments.

```text
R 0x1000
W 0x1040
R,0x1080
```

Operations are `R` (read) or `W` (write), case-insensitive. Addresses may be hexadecimal (`0x...`) or decimal. Each line models one cache-block access; unaligned addresses are mapped to their containing block.

## Examples

```bash
./build/cache_sim --capacity 1024 --block-size 64 --associativity 1 \
  --replacement LRU --trace traces/repeated_access.trace

./build/cache_sim --capacity 1024 --block-size 64 --associativity 4 \
  --replacement FIFO --trace traces/conflict_misses.trace \
  --csv results/conflict_fifo.csv
```

## Run tests

```bash
./build/cache_tests
```

The test suite covers:
- cold misses and repeated-access hits
- direct-mapped conflict misses
- set-associative behaviour
- LRU and FIFO replacement differences
- trace parsing and malformed input

## Compare configurations

Run the built simulator across a small configuration matrix:

```bash
python3 scripts/compare_configs.py \
  --simulator ./build/cache_sim \
  --trace traces/sequential.trace \
  --output results/sequential_comparison.csv
```

The script prints a table and saves CSV results. It expects the simulator to return a successful exit code for each configuration.

## Model assumptions and limitations

- One trace record represents one cache access; this tool does not split accesses that cross cache-block boundaries.
- A cache hit/miss is determined by tag presence only.
- Reads and writes are counted, but write-through/write-back policies, dirty bits, write allocation, prefetching, coherence, virtual addresses, TLBs, and multi-level caches are not modelled.
- No cycle timing or hardware-specific latency is estimated.
- LRU is exact for the configured set; FIFO evicts the oldest inserted line in the set.
- Cache capacity is data capacity and excludes metadata overhead.

## Suggested experiments

1. **Temporal locality:** Run `repeated_access.trace` and compare its hit rate with a trace that touches many distinct blocks.
2. **Conflict misses:** Compare direct-mapped and 4-way caches on `conflict_misses.trace`.
3. **Replacement policies:** Create a trace where LRU and FIFO evict different lines and compare their results.
4. **Associativity:** Hold capacity and block size constant while changing associativity.
5. **Block size:** Compare block sizes using the same byte addresses, noting that this simulator treats each record as one access.

## Repository structure

```text
CacheLab/
├── README.md
├── CMakeLists.txt
├── .gitignore
├── include/
├── src/
├── tests/
├── traces/
├── scripts/
└── results/
```
