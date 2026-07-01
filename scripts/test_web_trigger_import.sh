#!/bin/sh
set -eu

if ! command -v google-chrome >/dev/null 2>&1; then
    echo "google-chrome not found; skipping web trigger import check"
    exit 0
fi

./scripts/prepare_local_trigger_scenarios.sh >/dev/null

out_dir="${OUTDIR:-build_out}/web-trigger-import"
mkdir -p "$out_dir"

google-chrome \
    --headless \
    --disable-gpu \
    --no-sandbox \
    --dump-dom \
    --virtual-time-budget=2000 \
    'http://127.0.0.1:8088/?test=trigger-import' >"$out_dir/dom.html" 2>"$out_dir/chrome.err"

if grep -q "Uncaught\\|ReferenceError\\|TypeError" "$out_dir/chrome.err"; then
    cat "$out_dir/chrome.err" >&2
    exit 1
fi

grep -q "trigger import test done" "$out_dir/dom.html"
grep -q "slot20 RELAY 5 1" "$out_dir/dom.html"

