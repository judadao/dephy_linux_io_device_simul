#!/bin/sh
set -eu

test -f web/package.json
test -f web/index.html
test -f web/src/App.jsx
test -f web/src/main.jsx
test -f web/src/styles.css
test -f web/src/assets/device-panel.svg

grep -q '"@vitejs/plugin-react"' web/package.json
grep -q 'lucide-react' web/package.json
grep -q 'scriptEditor' web/src/App.jsx
grep -q 'site/demo/node/linux-sim-001/io' web/src/App.jsx
grep -q '@media' web/src/styles.css

