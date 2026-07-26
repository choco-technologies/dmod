---
name: dmod-coding-conventions
description: Coding conventions for DMOD module C code — heap-allocate strings/lists instead of fixed-size buffers (Dmod_StrDup, dmlist), never statically link a dependency into a module (always dmod_link_modules), keep functions under ~50 lines, and the _create/_destroy opaque-context pattern with a magic-field guard. Load this when writing or reviewing module implementation code (.c/.h files) in a dmod-* repo. Companion to the dmod-ecosystem skill.
---

# DMOD Module Coding Conventions

## Strings: prefer the heap over fixed-size buffers

The target is embedded (STM32F4/F7, ESP32-S3) — RAM is scarce and every byte
of it matters, so default to heap-allocating rather than reserving fixed
static/stack space "just in case". This applies to strings in general, not
just paths: don't declare a fixed-size `char buf[N]` for a string whose
length isn't tightly bounded — heap-allocate it instead. Beyond the general
RAM-budget argument, fixed-size stack/static buffers silently truncate or
overflow once a value gets longer than whoever wrote `N` expected, which is a
real risk for anything composed at runtime (paths, module names, mount
points under `/`, `.dmr`/`.dmm` entries, log/error messages, etc.). A static
fixed-size array is only acceptable when you can genuinely guarantee the
string will never exceed ~20 bytes (e.g. a short fixed enum-like tag) —
paths and other composed/runtime strings do not qualify.

When you need an owned copy of a string (not just a borrowed pointer), copy
it with `Dmod_StrDup` rather than `malloc` + `strcpy`/`memcpy`, and `free` it
when done.

The same reasoning applies to lists: don't hand-roll a fixed-size static
array for a collection whose size grows/shrinks at runtime. Use the `dmlist`
module instead — pull it in with `dmod_link_modules(${DMOD_MODULE_NAME}
dmlist)` in `CMakeLists.txt` (see `dmtty/CMakeLists.txt` for a working
example) rather than reimplementing a dynamic list.

## Never statically link libraries into a module

Always link a module against another module's `_if` target dynamically
(`dmod_link_modules`, the standard Built-in API / MAL / DIF mechanisms) —
never statically link a library's object code directly into a module. DMOD
modules are loaded/unloaded independently at runtime; statically linking a
dependency into a module defeats that (bloats the `.dmf`, duplicates the
dependency's RAM/flash footprint across every module that links it, and
breaks the loader's ability to resolve/share a single instance of it). If
you find yourself reaching for `target_link_libraries` with anything other
than a module's own `_if` interface target, stop and use
`dmod_link_modules` instead.

## Keep functions short and extract shared logic

Functions should stay short — **max ~50 lines**. If a function is growing
past that, it's a signal to split it, not to keep going. Always look for the
common part between two similar code paths (two callers doing almost the
same thing, two branches with overlapping logic, ...) and pull it into its
own function rather than duplicating it inline.

## Prefer the `_create`/`_destroy` context pattern

The preferred way to manage a piece of stateful data is a context object:
a `_create` function allocates and initializes a context struct holding all
the data that logic needs, callers thread that context pointer through the
rest of the API, and a `_destroy` function frees it once it's no longer
needed. See `dm_sw_ring_create()`/`dm_sw_ring_destroy()` in
`dm_sw_ring/include/dm_sw_ring.h` for a worked example of this shape.

**Context structs must be opaque/private.** Declare only the typedef in the
public/interface header, and define the actual struct body in the private
source file:

```c
/* public header (e.g. include/<module>.h) */
typedef struct context context_t;
```

```c
/* private file (e.g. src/<module>.c or a private header) */
struct context
{
    ...
};
```

This keeps callers from reaching into the struct directly or depending on
its layout — all access must go through the module's API functions.

**Guard context structs with a magic field.** Give the struct a `magic`
field (`uint32_t`) set in `_create` to a fixed value derived from a short
form of the module's name (e.g. a 4-char tag packed into a `uint32_t`), and
check it at the top of every function that receives the context pointer,
before the context is dereferenced (or used) — the check should be present
in every entry point (`_destroy` included), not just a subset. This catches
use of a stale/freed, corrupted, or wrong-type pointer early instead of
leading to undefined behavior further down.
