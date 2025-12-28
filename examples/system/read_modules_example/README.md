# Module Listing Example

This example demonstrates how to use the `Dmod_OpenModules`, `Dmod_ReadNextModule`, and `Dmod_CloseModules` functions to iterate through all available modules in the system.

## Description

These functions allow you to enumerate all modules that are available in:
- Filesystem paths (configured via environment variables and search paths)
- Loaded packages (.dmp files)

## Usage

The functions use an explicit open/close pattern to manage resources properly:

```c
// Allocate module node structure
Dmod_ModuleNode_t moduleNode;
moduleNode._Data = NULL;

// Open module iteration
if (!Dmod_OpenModules(&moduleNode))
{
    fprintf(stderr, "Failed to open module iteration\n");
    return 1;
}

// Iterate through all modules
while (Dmod_ReadNextModule(&moduleNode))
{
    // Access module information:
    // - moduleNode.path: Path to the module file
    // - moduleNode.header: Module header with name, version, etc.
    printf("Found module: %s at %s\n", moduleNode.header.Name, moduleNode.path);
}

// Close module iteration (important to free resources!)
Dmod_CloseModules(&moduleNode);
```

## Key Points

1. The user allocates the `Dmod_ModuleNode_t` structure
2. Call `Dmod_OpenModules` to initialize iteration
3. Call `Dmod_ReadNextModule` repeatedly while it returns `true`
4. **Always** call `Dmod_CloseModules` when done (even if stopping early) to free resources
5. The open/close pattern prevents memory leaks

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
