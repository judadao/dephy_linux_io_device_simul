# Module Structure

`linux_io_device_simul` follows the reusable module contract from
`dephy_module_golden_sample`.

## What Is Reused

- Golden sample repo layout, audit shape, Zephyr metadata stub, C11 Makefile
  style, docs/TODO convention, and public-header-first API organization.
- Industrial IO naming and payload conventions from `dephy_industrial_io`:
  DI/DO/AI/AO channel types, event-oriented samples, and
  `site/<site>/node/<node>/io/<channel>/<suffix>` topics.

## What Is New

- Linux-only script-driven simulator core.
- CLI that reads simple IO scripts from stdin or a file.
- Initial JSON event payloads that are protocol-friendly, without binding the
  repo to MQTT, Modbus, or a product-specific matrix demo.
- Vite + React local web dashboard for browser-based simulator demos.

## Near-Term Extension Points

- Add a transport adapter interface for MQTT/Modbus/TCP publishers.
- Add external script scenario files for matrix and fast-module demos.
- Add deterministic time and replay controls for benchmark runs.
- Connect the web dashboard to the native simulator through a local API or
  WebSocket bridge.
