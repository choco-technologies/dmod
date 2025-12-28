# Module Listing Example

This example demonstrates how to use the `Dmod_ReadNextModule` function to iterate through all available modules in the system.

## Description

The `Dmod_ReadNextModule` function allows you to enumerate all modules that are available in:
- Filesystem paths (configured via environment variables and search paths)
- Loaded packages (.dmp files)

## Usage

The function uses a stateful iterator pattern:

```c
// Allocate module node structure
Dmod_ModuleNode_t moduleNode;
moduleNode._Data = NULL;  // Initialize to NULL for first call

// Iterate through all modules
while (Dmod_ReadNextModule(&moduleNode))
{
    // Access module information:
    // - moduleNode.path: Path to the module file
    // - moduleNode.header: Module header with name, version, etc.
    printf("Found module: %s at %s\n", moduleNode.header.Name, moduleNode.path);
}
```

## Key Points

1. The user allocates the `Dmod_ModuleNode_t` structure
2. Set `_Data` to `NULL` before the first call
3. The function manages internal iteration state automatically
4. Returns `true` while modules are found, `false` when iteration is complete
5. Resources are automatically cleaned up when iteration finishes

## Module Information Available

For each module found, you can access:
- `header.Name`: Module name
- `header.Version`: Module version
- `header.Author`: Module author
- `header.Arch`: Target architecture
- `header.ModuleType`: Library or Application
- `path`: Full path or package notation (e.g., `[package]/module`)

## Building

This example is built as part of the DMOD examples when `DMOD_BUILD_EXAMPLES` is enabled.
