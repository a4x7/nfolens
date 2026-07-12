# nfolens

A lightweight terminal UI for real-time Linux system stats, built with [FTXUI](https://github.com/ArthurSonzogni/FTXUI). Reads directly from `/proc` — no external dependencies for data collection.

![screenshot-nfolens](https://github.com/a4x7/nfolens/blob/main/assets/image.png)

## Features

- **Hostname** — read from `/etc/hostname`
- **CPU usage** — computed from `/proc/stat` deltas, plus clock speed from `/proc/cpuinfo`
- **Memory usage** — total and used, from `/proc/meminfo`
- **Network throughput** — combined upload/download rate across `eth*`, `wl*`, and `en*` interfaces, from `/proc/net/dev`
- **Uptime** — from `/proc/uptime`
- Live-updating gauges with a green → yellow → red gradient based on load
- Runs on a background polling thread; the UI thread only re-renders on change

## Requirements

- Linux (relies on `/proc` and `/etc/hostname`)
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- CMake (recommended for pulling in FTXUI)

## Building

Using CMake with FTXUI fetched as a dependency:

```bash
mkdir build
cmake -S . -B ./build
cmake --build ./build
```


If you choose to build and link FTXUI yourself, ftxui-component must be first in the linking order relative to the other FTXUI libraries, i.e.

```bash
g++ ./src/main.cpp -lftxui-component -lftxui-dom -lftxui-screen -lpthread -o ./build/main
```

## Usage

```bash
./build/main [refresh_ms]
```

- `refresh_ms` — polling interval in milliseconds (default: `1000`)
- Press `q` to quit

Example, refreshing every 500 ms:

```bash
./main 500
```

## How it works

`nfolens` runs two threads:

1. **Worker thread** — polls `/proc/uptime`, `/proc/cpuinfo`, `/proc/meminfo`, `/proc/net/dev`, and `/proc/stat` on each interval, computes deltas (CPU busy time, network bytes/sec), and writes the results into a mutex-guarded shared struct.
2. **UI thread** — an FTXUI render loop that redraws whenever the worker thread posts a "changed" event, reading a locked snapshot of the shared state.

CPU usage is derived from the change in total vs. idle jiffies between two `/proc/stat` reads, rather than a single instantaneous read. Network rate is derived similarly from cumulative byte counters in `/proc/net/dev`.

## Notes / Limitations

- Currently Linux-only (depends on procfs).
- Network interfaces are matched by name prefix (`eth`, `wl`, `en`); interfaces named differently (e.g. custom VPN/tunnel names) won't be counted.
- Assumes a single CPU frequency line format from `/proc/cpuinfo`; behavior only tested on x86_64 architecture.
