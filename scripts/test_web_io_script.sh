#!/bin/sh
set -eu

if ! command -v google-chrome >/dev/null 2>&1; then
    echo "google-chrome not found; skipping web IO script check"
    exit 0
fi

out_dir="${OUTDIR:-build_out}/web-io-script"
mkdir -p "$out_dir"

google-chrome \
    --headless \
    --disable-gpu \
    --no-sandbox \
    --dump-dom \
    --virtual-time-budget=2000 \
    'http://127.0.0.1:8088/?test=io-script' >"$out_dir/dom.html" 2>"$out_dir/chrome.err"

if grep -q "Uncaught\\|ReferenceError\\|TypeError" "$out_dir/chrome.err"; then
    cat "$out_dir/chrome.err" >&2
    exit 1
fi

grep -q "script test done" "$out_dir/dom.html"
grep -q "S1/DI/1" "$out_dir/dom.html"
grep -q "S3/AI/2" "$out_dir/dom.html"
grep -q "S5/RELAY/3" "$out_dir/dom.html"
grep -q "site/demo/node/linux-sim-001/slot/5/io/relay/3/event" "$out_dir/dom.html"

