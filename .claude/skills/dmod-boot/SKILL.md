---
name: dmod-boot
description: Explains dmod-boot, the firmware entry point that initializes hardware/FreeRTOS/heap/VFS/logging and boots into a loaded module package — how module packaging (modules.dmp) works, how to test a locally-built .dmf on real hardware without a release, and the flash/connect/monitor CMake targets for working with real boards. Load this when working in the dmod-boot repo, tracing how firmware boots into a usable system, or flashing/debugging a board. Companion to the dmod-ecosystem skill.
---

# `dmod-boot`: the entry point

`dmod-boot` is the bootloader/firmware entry point for a real board. It
initializes hardware, FreeRTOS (via `dmosi-freertos`), heap (`dmheap`), VFS
(`dmvfs`, mounting `dmramfs` on `/`, `dmdevfs` on `/dev`), logging (`dmlog`),
then loads a package of modules embedded directly in ROM at link time and
starts a main module — typically the shell, `dmell`. If you're tracing "how
does firmware boot into a usable system", `dmod-boot/src/main.c` is the
starting point.

**Module packaging.** During the build, `dmod-boot` bundles every `.dmf` it
finds in `<dmod-boot>/build/dmf/` into a single `modules.dmp` package, which
is then embedded in flash alongside the rest of the firmware (`modules/modules.dmd`
is the manifest that determines which modules get fetched/built into
`build/dmf/` in the first place).

**Testing a locally-built module on real hardware, without a release:** drop
the `.dmf` binary straight into `<dmod-boot>/build/dmf/`, then delete
`<dmod-boot>/build/modules.dmp` and `<dmod-boot>/build/__modules_dmp.o` so the
next build is forced to regenerate the package (an incremental build won't
notice a `.dmf` was added/changed in place otherwise).

**Flashing and debugging** (from `<dmod-boot>/build`):
- `cmake --build . --target install-firmware` — flash the firmware to the board.
- `cmake --build . --target connect` — attach OpenOCD to the target.
- `cmake --build . --target monitor` — attach a console to `dmell` over the
  existing OpenOCD connection (run `connect` first). This gives you stdin/stdout
  independent of any firmware-level driver — you get a working shell even
  before a UART driver is loaded, since it doesn't go through `dmuart`/`dmtty`
  at all. Log output for this relies on `dmlog`, the base/kernel-level logging
  system used from very early boot (before most modules are up), which is why
  `dmod-boot` depends on it directly rather than through a loaded module.
