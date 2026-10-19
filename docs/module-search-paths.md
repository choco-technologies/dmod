# Module search paths

The loader combines `PATH`, the compiled `DMOD_DMF_DIR` and `DMOD_DMFC_DIR`,
compiled `DMOD_REPO_PATHS`, `Dmod_GetRepoDir()`, and the environment values
`DMOD_REPO_PATHS`, `DMOD_DMFC_DIR`, `DMOD_DMF_DIR`, in that order. Entries
are separated by `DMOD_ARRAY_SEP`. Existing precedence is preserved: the
last source and the last entry in a source are searched first.

An absolute entry can contain one complete `*` directory component, for
example `/mnt/*/bin`. It expands the directories directly below `/mnt`,
with matches searched in alphabetical order. `.` and `..` and files are
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

`Dmod_Hlp_PrepareModulesSearchNodes()` returns an immutable reference-counted
snapshot, shared by module lookup, completion and module enumeration. The
cache holds at most four configurations; its key contains the values of
all effective search sources, so separate environment contexts do not
reuse one another's paths. Repeated lookups with an unchanged configuration
perform neither directory enumeration nor list allocation. Changing any
source selects or builds the corresponding snapshot automatically.

After a successful mount or unmount, a filesystem manager should call:

```c
Dmod_InvalidateModuleSearchPaths();
```

Call it from task context after releasing filesystem/mount locks. It drops
the cache's references and advances its generation; the next lookup builds
lazily. Readers already using a snapshot retain it until they finish.
Builders invalidated while enumerating may finish their current lookup but
cannot publish a stale snapshot into the cache. Failed allocation does not
publish a partial snapshot. `Dmod_Deinitialize()` also releases the cache.

The cache stores directories, not file names or existence results: adding
or removing a `.dmf` or `.dmfc` does not require invalidation. An unmount
racing with a lookup can still make the file open fail normally. Mount
changes made outside automount must issue the same notification. As with
other uses of `Dmod_GetEnv`, callers must not concurrently mutate or free
the environment strings being read.

On pthread hosts the cache metadata uses a mutex; other targets use their
SAL critical-section implementation. No allocation, filesystem I/O or
memory freeing happens while that metadata lock is held.
