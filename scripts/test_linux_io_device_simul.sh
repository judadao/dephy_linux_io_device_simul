#!/bin/sh
set -eu

out="${OUTDIR:-build_out}/demo.out"
"${OUTDIR:-build_out}/linux_io_device_simul" scripts/demo_io_script.txt > "$out"

grep -q 'site/demo/node/linux-sim-001/io/door/event' "$out"
grep -q '"event":"rising"' "$out"
grep -q '"channel":"pressure"' "$out"
grep -q '"event":"write"' "$out"
grep -q '"event":"falling"' "$out"

