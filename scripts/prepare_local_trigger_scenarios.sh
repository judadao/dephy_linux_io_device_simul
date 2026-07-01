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

cat > "$local_dir/twenty_slot_interleaved.trigger" <<'SCRIPT'
# Interleaved 20-slot IO trigger scenario.
# Format: slotX IO_TYPE CHANNEL VALUE
slot1 DI 1 1
slot6 DI 2 1
slot11 DI 3 1
slot16 DI 4 1
sleep 25
slot2 DO 1 1
slot7 DO 2 1
slot12 DO 3 1
slot17 DO 4 1
sleep 25
slot3 AI 1 120
slot8 AI 2 240
slot13 AI 3 360
slot18 AI 4 480
sleep 25
slot4 AO 1 80
slot9 AO 2 160
slot14 AO 3 240
slot19 AO 4 320
sleep 25
slot5 RELAY 1 1
slot10 RELAY 2 1
slot15 RELAY 3 1
slot20 RELAY 4 1
sleep 50
slot1 DI 1 0
slot2 DO 1 0
slot3 AI 1 0
slot4 AO 1 0
slot5 RELAY 1 0
sleep 25
slot6 DI 2 0
slot7 DO 2 0
slot8 AI 2 0
slot9 AO 2 0
slot10 RELAY 2 0
sleep 25
slot11 DI 3 0
slot12 DO 3 0
slot13 AI 3 0
slot14 AO 3 0
slot15 RELAY 3 0
sleep 25
slot16 DI 4 0
slot17 DO 4 0
slot18 AI 4 0
slot19 AO 4 0
slot20 RELAY 4 0
sleep 50
slot1 DI 16 1
slot2 DO 15 1
slot3 AI 14 1400
slot4 AO 13 1300
slot5 RELAY 12 1
slot6 DI 11 1
slot7 DO 10 1
slot8 AI 9 900
slot9 AO 8 800
slot10 RELAY 7 1
slot11 DI 6 1
slot12 DO 5 1
slot13 AI 4 400
slot14 AO 3 300
slot15 RELAY 2 1
slot16 DI 1 1
slot17 DO 2 1
slot18 AI 3 300
slot19 AO 4 400
slot20 RELAY 5 1
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

cp "$local_dir/basic_io.trigger" "$public_dir/basic_io.trigger"
cp "$local_dir/import_export_run.trigger" "$public_dir/import_export_run.trigger"
cp "$local_dir/twenty_slot_interleaved.trigger" "$public_dir/twenty_slot_interleaved.trigger"

cat > "$local_dir/README.local.txt" <<'TXT'
This directory is intentionally ignored by git.

Files:
- basic_io.trigger: plain trigger script for manual copy/paste.
- import_export_run.trigger: trigger script used by the import/export smoke scene.
- twenty_slot_interleaved.trigger: 20-slot cross-trigger stress scenario.

Served import fixture:
- web/public/local_trigger_scenarios/import_export_run.json
- web/public/local_trigger_scenarios/*.trigger

Regenerate:
  ./scripts/prepare_local_trigger_scenarios.sh
TXT

echo "Prepared local trigger scenarios:"
echo "  $local_dir"
echo "  $public_dir"
