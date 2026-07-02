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

For motion prediction integration, the CLI also supports slot trigger scripts:

```txt
slot1 DI 1 1
slot3 AI 1 80
slot3 AI 2 65
slot5 RELAY 1 0
sleep 300
slot1 DI 2 1
```

Run the slot stream mode with a loop and a simulated IO sample period:

```sh
build_out/linux_io_device_simul --slot-stream --loop 2 --sample-ms 300 scripts/motion_pipeline.trigger
```

This emits one JSON event per slot command:

```txt
site/demo/node/linux-sim-001/slot/3/io/ai/1/event {"event":"changed","slot":3,"type":"ai","channel":1,"value":80,"t_ms":900,"loop":1}
```

`dephy_ml_high_speed_implement` can consume that stream directly and predict
the missing high-rate joint frames between these slow IO anchors.

For the single-palm demo, the simulator can stream hand keyframes:

```txt
hand open_start 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0 0.012 0
hand half_close 0.10 0.04 0.02 0.18 -0.06 0.08 0.45 0 0.012 0
```

Run:

```sh
build_out/linux_io_device_simul --hand-stream --loop 2 --sample-ms 300 scripts/hand_keyframe_demo.script
```

The stream emits one JSON keyframe anchor every simulated 300ms. The implement
repo loads these anchors and applies its loaded prediction policy between them.

## Recording Hand Keyframes

The simulator can also record hand keyframe events back into CSV. This is the
intended workflow before training:

```txt
real IO or device simulator
  -> hand keyframe event stream
  -> recorded keyframe CSV
  -> dephy_ml_high_speed_implement training / prediction
```

For the current simulator-only demo:

```sh
build_out/linux_io_device_simul \
  --hand-stream \
  --loop 1 \
  --sample-ms 300 \
  scripts/hand_keyframe_demo.script \
  > build_out/hand_device_stream.out

build_out/linux_io_device_simul \
  --record-hand-keyframes \
  build_out/hand_device_stream.out \
  > build_out/recorded_hand_keyframes.csv
```

The recorded CSV matches the implement repo's hand keyframe format:

```txt
frame_id,t_ms,x,y,z,yaw,pitch,roll,grip,hold_ms,tolerance,safety_hold
open_start,0,0.00000,0.00000,0.00000,0.00000,0.00000,0.00000,0.00000,0,0.01200,0
```

That file can then be used by `dephy_ml_high_speed_implement`:

```sh
../dephy_ml_high_speed_implement/build_out/dephy_hand_predict \
  --keyframes build_out/recorded_hand_keyframes.csv \
  --policy ../dephy_ml_high_speed_implement/examples/hand/hand_policy.json \
  --render-ms 16
```

In the later real-device flow, actual IO values will usually jitter. The
recorded keyframes remain the low-rate anchors, and the implement side uses its
trained policy plus the current observed state to keep correcting direction and
fill the missing frames until each keyframe is reached smoothly.

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
make -f Makefile.linux prepare-local-trigger-scenarios
make -f Makefile.linux web-render-check
make -f Makefile.linux web-io-script-check
make -f Makefile.linux web-import-export-run-check
make -f Makefile.linux web-trigger-import-check
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

Trigger controls:

- `Step` executes one script line.
- `Start Loop` executes the script repeatedly using the configured loop count.
- `Infinite` keeps looping until `Stop`.
- `Stop` cancels the active loop immediately.

The web IO script check opens `http://127.0.0.1:8088/?test=io-script` and
verifies that a DI, AI, and RELAY command produce event log output.

Local trigger scenarios are generated into ignored folders:

```sh
make -f Makefile.linux prepare-local-trigger-scenarios
```

Generated locations:

- `local_trigger_scenarios/`: plain trigger scripts for manual copy/paste.
- `web/public/local_trigger_scenarios/`: JSON scenes that the local web server
  can import for smoke tests.

`make -f Makefile.linux web-import-export-run-check` generates the local
scenario JSON, loads it through the dashboard import path, validates export
shape, runs the trigger script, and checks the resulting event log.

`Import` accepts both full scene JSON files and plain `.trigger` / `.txt`
trigger scripts. Plain trigger scripts are loaded into the Trigger Script
editor without replacing the slot config.
