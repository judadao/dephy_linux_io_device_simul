# linux_io_device_simul

Linux IO device simulator for script-driven digital/analog IO inputs and
industrial protocol friendly event payloads.

The intended path is to use this repo as the Linux-side simulated device in
demos that combine IO devices, matrix logic, and fast reusable modules.

## What Can Be Reused

- Use `dephy_module_golden_sample` for repo structure, audit rules, C11 build
  style, Zephyr metadata shape, and TODO/doc conventions.
- Use `dephy_industrial_io` as the domain reference for DI/DO/AI/AO naming,
  event samples, topic shape, JSON payloads, and future Modbus/MQTT adapters.

## What Should Be New Here

- Linux-specific simulator CLI and scenario runner.
- Script/replay support for deterministic IO input changes.
- Protocol output adapters used by Linux demos and tests.

## Build And Test

```sh
make -f Makefile.linux
make -f Makefile.linux test
make -f Makefile.linux demo
```

## Script Format

```txt
sleep 10
set door 1
set pressure 1234
write relay 1
set door 0
```

Supported commands:

- `sleep <ms>` advances simulator time.
- `set <input-channel> <value>` changes a DI/AI input.
- `write <output-channel> <value>` changes a DO/AO output.
- `poll` is reserved for future transport and timed behavior.

## Current CLI Output

The CLI prints one line per event:

```txt
site/demo/node/linux-sim-001/io/door/event {"event":"rising","channel":"door","type":"di","value":1,"fault":0,"t_ms":10}
```

This is intentionally transport-neutral so later demos can route the same event
stream to MQTT, Modbus/TCP, files, or a matrix engine.

## Local Web Simulator

The `web/` directory contains a Vite + React dashboard for local Linux demos.

```sh
make -f Makefile.linux web-install
make -f Makefile.linux web
```

Then open `http://127.0.0.1:8088/`.

Useful web commands:

```sh
make -f Makefile.linux web-build
make -f Makefile.linux web-render-check
npm --prefix web run dev -- --host 127.0.0.1 --port 8088
```

The dashboard models up to 20 swappable IO module slots. Each module slot has
16 channels. Digital and relay channels light when value is `1`; analog
channels light when value is greater than `0`.

Web script commands:

```txt
add slot6 DI
slot1 DI 1 1
slot3 AI 1 128
slot5/RELAY/1/0
remove slot6
```

Command fields are `slotX`, `io type`, `channel number`, and `value`. Slash and
space separated forms are both supported.

Use `Export` to download a JSON scene containing module slots, channel values,
and the script. Use `Import` to load the same JSON later for fast test scenario
switching.
