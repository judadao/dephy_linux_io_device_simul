#!/bin/sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
local_dir="$repo_root/local_trigger_scenarios"
public_dir="$repo_root/web/public/local_trigger_scenarios"

mkdir -p "$local_dir" "$public_dir"

cat > "$local_dir/basic_io.trigger" <<'SCRIPT'
slot1 DI 1 1
slot1 DI 2 1
slot3 AI 2 42
slot5 RELAY 3 1
sleep 50
slot1 DI 1 0
slot5 RELAY 3 0
SCRIPT

cat > "$local_dir/import_export_run.trigger" <<'SCRIPT'
slot1 DI 1 1
slot3 AI 2 42
slot5 RELAY 3 1
sleep 25
slot2 DO 4 1
SCRIPT

cat > "$public_dir/import_export_run.json" <<'JSON'
{
  "version": 1,
  "name": "import-export-run-smoke",
  "modules": [
    { "slotNo": 1, "type": "di" },
    { "slotNo": 2, "type": "do" },
    { "slotNo": 3, "type": "ai" },
    { "slotNo": 5, "type": "relay" }
  ],
  "channels": [
    { "slot": 1, "type": "DI", "channel": 8, "value": 1 },
    { "slot": 3, "type": "AI", "channel": 9, "value": 77 }
  ],
  "script": "slot1 DI 1 1\nslot3 AI 2 42\nslot5 RELAY 3 1\nsleep 25\nslot2 DO 4 1"
}
JSON

cat > "$local_dir/README.local.txt" <<'TXT'
This directory is intentionally ignored by git.

Files:
- basic_io.trigger: plain trigger script for manual copy/paste.
- import_export_run.trigger: trigger script used by the import/export smoke scene.

Served import fixture:
- web/public/local_trigger_scenarios/import_export_run.json

Regenerate:
  ./scripts/prepare_local_trigger_scenarios.sh
TXT

echo "Prepared local trigger scenarios:"
echo "  $local_dir"
echo "  $public_dir"

