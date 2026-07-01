# IO-device simulator trigger script for motion prediction integration.
# Format: slotX IO_TYPE CHANNEL VALUE
slot1 DI 1 1
slot1 DI 2 0
slot1 DI 3 0
slot3 AI 1 80
slot3 AI 2 65
slot3 AI 3 70
slot3 AI 4 75
slot3 AI 5 20
slot3 AI 6 25
slot3 AI 7 70
slot3 AI 8 30
slot3 AI 9 55
slot3 AI 10 50
slot3 AI 11 52
slot3 AI 12 48
slot3 AI 13 95
slot3 AI 14 70
slot3 AI 15 65
slot3 AI 16 55
slot5 RELAY 1 0
slot5 RELAY 2 1
slot5 RELAY 3 1
slot5 RELAY 4 1
slot5 RELAY 5 1
slot5 RELAY 6 1
slot5 RELAY 7 1
slot5 RELAY 8 0
sleep 300
slot1 DI 2 1
slot3 AI 1 60
slot3 AI 11 42
slot3 AI 12 62
slot5 RELAY 5 1
sleep 300
slot1 DI 2 0
slot1 DI 3 1
slot3 AI 1 90
slot3 AI 11 58
slot3 AI 12 44
