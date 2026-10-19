# Module search paths

The loader combines `PATH`, the compiled `DMOD_DMF_DIR` and `DMOD_DMFC_DIR`,
compiled `DMOD_REPO_PATHS`, `Dmod_GetRepoDir()`, and the environment values
`DMOD_REPO_PATHS`, `DMOD_DMFC_DIR`, `DMOD_DMF_DIR`, in that order. Entries
are separated by `DMOD_ARRAY_SEP`. Existing precedence is preserved: the
last source and the last entry in a source are searched first.

An absolute entry can contain one complete `*` directory component, for
example `/mnt/*/bin`. It expands the directories directly below `/mnt`,
in the order returned by the filesystem (last added is searched first). `.` and `..` and files are
excluded. The literal suffix need not exist yet: a card mounted without
`bin` can receive it and new tools through an upload later. Relative
wildcards, partial-component patterns and multiple stars are unsupported
and are skipped with a warning. Literal relative paths keep their existing
meaning relative to the caller's current directory.

For flash to take precedence over cards, use:

```
DMOD_REPO_PATHS=/mnt/*/bin:/flash/dmf:/flash/dmfc
```

## Cache and hotplug

`Dmod_Hlp_PrepareModulesSearchNodes()` builds the linked list once and stores
its tail in a global variable. Subsequent calls return that tail without
reading configuration, allocating nodes or scanning directories.

After a successful mount/unmount or a change to the search path configuration,
call from task context, after releasing filesystem/mount locks:

```c
Dmod_InvalidateModuleSearchPaths();
```

Invalidation sets the global tail to NULL; the next lookup rebuilds the list.
The tail counts references from the cache and its readers, so invalidation
does not free nodes still used by a lookup or module iterator. Callers release
the list through `Dmod_Hlp_FreeSearchPathList()` and must not modify it.
There is one system-wide cache; environment changes require explicit
invalidation, including when switching between different environments.

The cache stores directories, not file names: uploading or removing a `.dmf`
or `.dmfc` does not require invalidation. A file open may still fail if the
card is unmounted during a lookup.

The SAL mutex serializes cache construction, access and invalidation.
`Dmod_Initialize()` creates it and `Dmod_Deinitialize()` releases the cache
and deletes it. Initialization/deinitialization require quiescent callers.
Lookups before initialization or without SAL mutex support remain uncached.
