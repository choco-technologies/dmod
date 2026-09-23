# DMOD file I/O

The canonical seek, tell, size, and stat path uses explicit 64-bit types:

```c
typedef int64_t Dmod_FileOffset_t;
typedef uint64_t Dmod_FileSize_t;

typedef struct {
    Dmod_FileSize_t Size;
    uint32_t Mode;
} Dmod_FileStat_t;
```

The corresponding Built-in APIs are version 2.0:

```c
int Dmod_FileSeek(void* file, Dmod_FileOffset_t offset, int origin);
Dmod_FileOffset_t Dmod_FileTell(void* file);
Dmod_FileSize_t Dmod_FileSize(void* file);
int Dmod_FileStat(const char* path, Dmod_FileStat_t* stat);
```

`Dmod_FileTell()` returns `DMOD_FILE_OFFSET_ERROR` on failure.
`Dmod_FileSize()` returns zero for an empty file or on failure and preserves the
current file position. Call `Dmod_FileStat()` when the caller must distinguish
an empty file from an error; it returns zero on success and `-1` on failure,
following the C/POSIX stat convention.

## Platform boundary

The default weak implementation uses `_fseeki64`/`_ftelli64` on Windows,
`fseeko`/`ftello` with 64-bit `off_t` on POSIX, and checked `fseek`/`ftell`
fallbacks elsewhere. If a requested offset cannot be represented by the host
stdio type, the operation fails with `errno` set to `EOVERFLOW`; it never
silently narrows or wraps.

Systems providing their own VFS must implement all four version-2.0 APIs with
the exact types above. Conversions to a filesystem or libc-specific offset type
must be checked in both directions.

## Compatibility and migration

This is an ABI-breaking replacement of the old `long`/`size_t` seek path.
Modules that call these APIs must rebuild against the new `dmod_sal.h`.
Providers must publish version-2.0 signatures; a provider that still exports a
version-1.0 signature will not be connected by the loader.
