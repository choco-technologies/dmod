# Absolute pointer relocations on ARM and x86-64

PIC alone does not relocate pointers embedded in static aggregates. GCC can
also create such aggregates while optimizing a local structure initializer.
Callbacks and third-party constant dispatch tables can therefore contain link
offsets even when every instruction is position independent.

After `dmod_setup_external_module()`, call `dmod_enable_pointer_relocations()`
before defining modules. The SDK preserves ELF relocation metadata and
records R_ARM_ABS32/R_ARM_TARGET1 or R_X86_64_64 pointer locations in the DMF. It leaves the
existing header, API, GOT and license relocations to their existing handlers.
Non-pointer integers are not guessed or scanned for possible addresses.
Unsupported absolute instruction relocations fail the build.

The implementation supports ARM ELF32 and x86-64 ELF64 and needs Python 3 at build time. Linker relaxation is disabled so the static link cannot turn GOT references into absolute instruction immediates. Use host tools and a
runtime built from an SDK supporting this feature. An opted-in module has a
256-byte header: the unchanged 240-byte legacy header followed by a 16-byte
extension (magic 0x524c4f43, version 1, table byte offset, record count). The
legacy HeaderSize and footer header-section size describe the extended header.
The table is appended after the original binary image and contains sorted
32-bit byte offsets. Each identifies a pointer-sized module-relative value.
The loader validates the entire table before applying it after the existing
section relocations. Thumb function pointer bits are preserved. Crossplatform
inspection validates the directory without writing native-size pointers.

Older loaders reject the extended HeaderSize. Legacy modules retain their
original format and still load with the updated runtime. This is opt-in and
currently limited to ARM ELF32 and x86-64 ELF64; other architectures need their own reviewed
relocation mapping before enabling it.
