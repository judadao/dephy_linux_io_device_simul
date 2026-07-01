#!/bin/sh
set -eu

if [ "${1:-}" = "--metadata-only" ]; then
    test -f zephyr/module.yml
    test -f zephyr/Kconfig
    test -f zephyr/CMakeLists.txt
    grep -q 'name: linux_io_device_simul' zephyr/module.yml
    exit 0
fi

echo "Only --metadata-only is supported in this Linux simulator repo." >&2
exit 2

