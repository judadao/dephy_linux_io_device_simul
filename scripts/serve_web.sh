#!/bin/sh
set -eu

cd "$(dirname "$0")/.."

if [ ! -d web/node_modules ]; then
    npm --prefix web install
fi

exec npm --prefix web run dev -- --host "${HOST:-127.0.0.1}" --port "${PORT:-8088}"

