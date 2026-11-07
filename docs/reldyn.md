# DMOD_USE_RELDYN: the table of the relocations in place of the GOT

## Why

A module is linked for the address 0 and loaded at any address. On the ARM the compiler makes the code position independent
(`-fPIC`): the addresses go through the GOT, and the loader adds the address of the load to each entry of it. GCC for the Xtensa
has no such mode: the code loads the addresses it needs as constants from the literal pools (`l32r`), and nothing in the module
changes them when the module is loaded elsewhere. The GOT of such a module is empty.

For an architecture like that `DMOD_USE_RELDYN` is switched on. The section `.got` then has **the table of the relocations**:
the magic `1`, the number of the offsets and the offsets (32 bits each, ascending) of the words in the module
that hold an address. The loader adds the address of the load to each of those words.

The linker can still make entries of the GOT for some references (the addresses that the code takes through it, e.g. the
addresses of strings); they are in the same section, before the table, and have the address of the load added too. That is why the
table starts with the magic: the loader finds it among them. A section without the magic (a module built before it) is the table
only. The header, the footer and the layout of the module do not change, so a module of an
architecture without `DMOD_USE_RELDYN` is loaded as before.

## How it works

- `DMOD_USE_RELDYN` is an option of CMake (`OFF` unless the profile of the architecture sets it: `configs/arch/<arch>/<cpu>/tools-cfg.cmake`)
  and a macro (the same profile adds `-DDMOD_USE_RELDYN=1` to the flags - the loader must be built with it, like `DMOD_ARCH`).
- `dmod_create_module` (`scripts/CMakeLists.txt`) links such a module twice. The first link (`<module>_reldyn_probe`, the same sources and
  options, `-Wl,-q`) keeps the relocations in the ELF. `scripts/reldyn.cmake` reads them with `readelf` and writes `<module>_reldyn.c`:
  the table, in the section `.got`. The module is linked with it. The table is behind the code and the data, so it moves nothing
  of them: the offsets are the same in both links.
- The table has the words that hold an address (`R_XTENSA_32`, `R_XTENSA_PLT`). It does not have the words in `.header`, `.inputs`
  and `.outputs`: the loader relocates them itself (`Dmod_Ldr_LoadHeader/LoadInput/LoadOutput`).
- `Dmod_Ldr_LoadGot` (`dmod_ldr.c`) with `DMOD_USE_RELDYN` adds the address of the load to the words that the table points at.

## The code is executed at another address than it is written at

The internal RAM of the ESP32-S3 is seen by two buses: the data bus reads and writes it, the instruction bus executes it, at
another address (a jump to an address of the data bus is an `InstructionFetchError`). The loader writes a module through the
data bus and the code has to be run through the instruction bus, so the pointers to the code are the address of the instruction
bus:

- the profile lists such memories in `DMOD_CODE_ALIAS`, as `low:high:offset` separated by `;` (the ESP32-S3: `0x3FC88000:0x3FCF0000:0x6F0000`);
- `CMakeLists.txt` turns them into the macro `DMOD_CODE_ALIAS_MAP` for the loader;
- `Dmod_Hlp_CodeAddress()` (`dmod_hlp.c`, weak: a platform can replace it) gives the address that the code written at an address
  is executed from: `address + offset` when it is in `[low, high)`, the same address elsewhere;
- the loader uses it for the entry points of the header (`Preinit`, `Init`, `Main`, `Deinit`, `Signal`), the functions of
  `.inputs` and the words of the table that point into `.text`; the data stays at the data bus;
- the default `Dmod_SyncCode()` on the Xtensa makes the stores done and drops what the processor has fetched (`memw`, `isync`);
  the internal RAM has no cache. A memory with a cache in front of it (the external RAM of the ESP32-S3) needs the maintenance
  of that cache, which is the chip's and not the architecture's: the platform replaces `Dmod_SyncCode()` (dmod-boot:
  `src/arch/xtensa/lx7/caches.c`). The window of the data bus of the cache (0x3C000000..0x3E000000) is a second range of the alias in
  the profile of the ESP32-S3: the code is executed 0x6000000 higher.

## Packages built without relocation support

A loader built with `DMOD_USE_RELDYN` rejects a module whose `.got` section is
missing or empty. Even a new module with no relocations carries the magic and
a zero count, so it can still be loaded. Nonempty legacy tables without the
magic remain supported.

An example of an incompatible package is `memory` 0.9 for ESP32-S3: it has an
empty `.got` and calls the unrelocated `print_usage` address (`0x1a4`). Rebuild
and redistribute that module with the current Xtensa profile; accepting the
old binary would stop the processor in its exception handler.

## Other architectures

An architecture with the same problem sets `DMOD_USE_RELDYN` in its profile and adds the relocation types of its own to
`scripts/reldyn.cmake` (now `R_XTENSA_32` and `R_XTENSA_PLT`).
