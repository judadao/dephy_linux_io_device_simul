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

recorded_hand="${OUTDIR:-build_out}/recorded_hand_keyframes.csv"
"${OUTDIR:-build_out}/linux_io_device_simul" --record-hand-keyframes "$hand_out" > "$recorded_hand"
grep -q 'frame_id,t_ms,x,y,z,yaw,pitch,roll,grip,hold_ms,tolerance,safety_hold' "$recorded_hand"
grep -q 'open_start,0,0.00000,0.00000,0.00000' "$recorded_hand"
grep -q 'closed_reach,600,0.22000,0.08000,0.04000' "$recorded_hand"
wc -l "$recorded_hand" | awk '{ if ($1 != 11) exit 1; }'
