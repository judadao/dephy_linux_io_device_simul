#!/bin/sh
set -eu

if ! command -v google-chrome >/dev/null 2>&1; then
    echo "google-chrome not found; skipping web import/export/run check"
    exit 0
fi

./scripts/prepare_local_trigger_scenarios.sh >/dev/null

out_dir="${OUTDIR:-build_out}/web-import-export-run"
mkdir -p "$out_dir"

google-chrome \
    --headless \
    --disable-gpu \
    --no-sandbox \
    --dump-dom \
    --virtual-time-budget=3000 \
    'http://127.0.0.1:8088/?test=import-export-run' >"$out_dir/dom.html" 2>"$out_dir/chrome.err"

if grep -q "Uncaught\\|ReferenceError\\|TypeError" "$out_dir/chrome.err"; then
    cat "$out_dir/chrome.err" >&2
    exit 1
fi

grep -q "import export run test done" "$out_dir/dom.html"
grep -q "S1/DI/1" "$out_dir/dom.html"
grep -q "S2/DO/4" "$out_dir/dom.html"
grep -q "S3/AI/2" "$out_dir/dom.html"
grep -q "S5/RELAY/3" "$out_dir/dom.html"
grep -q "site/demo/node/linux-sim-001/slot/2/io/do/4/event" "$out_dir/dom.html"

google-chrome \
    --headless \
    --disable-gpu \
    --no-sandbox \
    --screenshot="$out_dir/page.png" \
    --window-size=1440,1000 \
    --virtual-time-budget=3000 \
    'http://127.0.0.1:8088/?test=import-export-run' >/dev/null 2>>"$out_dir/chrome.err"

bytes="$(wc -c < "$out_dir/page.png")"
if [ "$bytes" -lt 20000 ]; then
    echo "web import/export/run screenshot looks blank: $bytes bytes" >&2
    exit 1
fi
