#!/bin/sh
set -eu

outdir="${OUTDIR:-build_out}/hand_keyframe_script"
stream="$outdir/hand_stream.out"
csv="$outdir/recorded_hand_keyframes.csv"

mkdir -p "$outdir"

"${OUTDIR:-build_out}/linux_io_device_simul" \
    --hand-stream \
    --loop 1 \
    --sample-ms 300 \
    scripts/hand_keyframe_demo.script \
    > "$stream"

grep -q 'site/demo/node/linux-sim-001/hand/keyframe/event' "$stream"
grep -q '"event":"keyframe"' "$stream"
grep -q '"frame_id":"open_start"' "$stream"
grep -q '"frame_id":"closed_reach"' "$stream"
grep -q '"t_ms":600' "$stream"
test "$(wc -l < "$stream")" -eq 5

"${OUTDIR:-build_out}/linux_io_device_simul" \
    --record-hand-keyframes \
    "$stream" \
    > "$csv"

grep -q 'frame_id,t_ms,x,y,z,yaw,pitch,roll,grip,hold_ms,tolerance,safety_hold' "$csv"
grep -q 'open_start,0,0.00000,0.00000,0.00000' "$csv"
grep -q 'closed_reach,600,0.22000,0.08000,0.04000' "$csv"
test "$(wc -l < "$csv")" -eq 6
