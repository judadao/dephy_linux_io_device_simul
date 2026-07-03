#!/bin/sh
set -eu

grep -Fq 'const eventLogLimit = 15;' web/src/App.jsx
grep -Fq '.slice(0, eventLogLimit)' web/src/App.jsx
if grep -Fq '.slice(0, 100)' web/src/App.jsx; then
    echo "web event log limit regressed to 100" >&2
    exit 1
fi
