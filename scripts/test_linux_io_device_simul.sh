#!/bin/sh
set -eu

out="${OUTDIR:-build_out}/demo.out"
"${OUTDIR:-build_out}/linux_io_device_simul" scripts/demo_io_script.txt > "$out"

grep -q 'site/demo/node/linux-sim-001/io/door/event' "$out"
grep -q '"event":"rising"' "$out"
grep -q '"channel":"pressure"' "$out"
grep -q '"event":"write"' "$out"
grep -q '"event":"falling"' "$out"

slot_out="${OUTDIR:-build_out}/slot_stream.out"
"${OUTDIR:-build_out}/linux_io_device_simul" --slot-stream --loop 2 --sample-ms 300 scripts/motion_pipeline.trigger > "$slot_out"
grep -q 'slot/1/io/di/1/event' "$slot_out"
grep -q '"slot":1' "$slot_out"
grep -q '"type":"di"' "$slot_out"
grep -q '"channel":1' "$slot_out"
grep -q '"loop":2' "$slot_out"

hand_out="${OUTDIR:-build_out}/hand_stream.out"
"${OUTDIR:-build_out}/linux_io_device_simul" --hand-stream --loop 2 --sample-ms 300 scripts/hand_keyframe_demo.script > "$hand_out"
grep -q 'hand/keyframe/event' "$hand_out"
grep -q '"frame_id":"open_start"' "$hand_out"
grep -q '"grip":1.00000' "$hand_out"
grep -q '"loop":2' "$hand_out"
