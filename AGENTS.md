# Repository Guidelines

## Scope

This repository contains the Linux-side IO device simulator module. Keep the
simulator reusable and product-neutral. Product demos may depend on this module,
but product-specific matrix or workflow logic should stay in product repos.

## Layout

- `include/linux_io_device_simul/`: public simulator API.
- `src/`: reusable C11 implementation.
- `tools/`: Linux command-line simulator entry points.
- `tests/`: focused C unit tests.
- `scripts/`: audit and integration scripts.
- `docs/`: design notes and TODO tracking.
- `zephyr/`: metadata stub to preserve the reusable module contract.

## Commands

- `make -f Makefile.linux` builds the static library and CLI.
- `make -f Makefile.linux test` runs unit, script, structure, and metadata checks.
- `make -f Makefile.linux demo` runs the bundled script-driven demo.
- `make -f Makefile.linux web-install` installs the Vite/React web dependencies.
- `make -f Makefile.linux web` starts the local dashboard.

## Style

Use C11, `-Wall -Wextra`, four-space indentation, snake_case identifiers, and
uppercase macros. Do not add generated build output to git.

For `web/`, use Vite + React components, keep the first screen as the usable
simulator dashboard, and avoid product-specific demo logic until a product repo
depends on the module.
