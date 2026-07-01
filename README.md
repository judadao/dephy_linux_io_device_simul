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

