/* Linker script for host-side tools (dmod_add_tool(), e.g. dmf-get, dmfc,
 * todmfc). Unlike scripts/main.ld (written for freestanding/embedded
 * targets - fixed load address, no PHDRS/segment separation, no dynamic
 * linking), tools here are normal Linux userspace executables and must use
 * the toolchain's regular default layout (proper PIE, RELRO, separate
 * RO/RX/RW segments) to link and run correctly - a fixed low origin and a
 * single combined segment produces a malformed dynamically-linked
 * executable that (observed on aarch64) corrupts registers/return addresses
 * during early libc calls.
 *
 * This script only *adds* the .inputs/.outputs sections (DMOD's variable
 * system, see dmod-common.ld) into the linker's own default script via
 * INSERT, instead of replacing the whole SECTIONS layout.
 */
SECTIONS
{
    INCLUDE dmod-common.ld
}
INSERT AFTER .data;
