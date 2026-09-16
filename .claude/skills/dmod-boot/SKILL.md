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

**Module packaging.** During the build, `dmod-boot` bundles the modules it
finds in `<dmod-boot>/build/dmf/` into a single `modules.dmp` package, which
is then embedded in flash alongside the rest of the firmware (`modules/modules.dmd`
is the manifest that determines which modules get fetched/built into
`build/dmf/` in the first place). A module can be present there in two forms:
`<name>.dmf` (plain) and `<name>.dmfc` (the same module, fastlz-compressed).
`dmf-get` normally downloads both, and **the `.dmfc` takes precedence when the
module is loaded**.

**Testing a locally-built module on real hardware, without a release:** drop
the `.dmf` binary straight into `<dmod-boot>/build/dmf/`, **delete that
module's `<name>.dmfc` from the same directory**, then delete
`<dmod-boot>/build/modules.dmp` and `<dmod-boot>/build/__modules_dmp.o` so the
next build is forced to regenerate the package (an incremental build won't
notice a `.dmf` was added/changed in place otherwise).

Skipping the `.dmfc` step is the classic trap: the build succeeds, the new
`.dmf` really is in `build/dmf/`, and the board still silently runs the old
released module. It usually surfaces as a *dependency* error rather than an
obvious "stale module" one — the released build was linked against older
versions of its dependencies, so you get
`Module version mismatch: <x> != <y>` and
`API '<api>@<dep>:<x>/<y>' is not connected`, followed by the module failing
to enable. Two things make this hard to see: `strings build/modules.dmp` will
*not* find the offending signature (it is compressed inside the `.dmfc`), and
comparing `.dmf`/`.dmfc` timestamps in `build/dmf/` is the quickest way to
spot it — every module should have a matching pair, so the one module whose
two files disagree is the culprit.

**Beware of reconfiguring while testing a local module.** Re-running
`cmake -S . -B <build>` (as opposed to `cmake --build <build>`) re-runs
`dmf-get`, which re-downloads *every* module into `build/dmf/` and overwrites
any `.dmf` you dropped there by hand — and restores the `.dmfc` you deleted.
After any reconfigure, redo the steps above. Note also that `dmf-get` resolves
each repo's dependencies independently, so a refresh can legitimately leave
the set internally inconsistent (e.g. a command module still built against an
older version of a library module it uses); such a module fails to bind on its
own, which is harmless to the rest of the boot, but it is worth telling apart
from the stale-`.dmfc` case above.

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
