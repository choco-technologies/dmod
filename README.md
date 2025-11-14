<div style="width: 300px;">
  <img src="gimp/logo/Logo-Transparent.png" style="width: 100%;">
</div>

_________


The **Dmod (Dynamic Modules)** library allows you to add the functionality of loading programs and libraries into your **embedded** architecture in runtime mode. This means that you can dynamically extend the capabilities of your embedded system without needing to recompile or restart the entire application. 

### Key Features:
- **Dynamic Loading**: Load and unload modules at runtime.
- **Modular Architecture**: Design your system in a modular way, making it easier to manage and extend.
- **Inter-Module Communication**: Modules can communicate with each other and the system using a common API.
- **Resource Management**: Efficiently manage resources and dependencies between modules.
- **Cross-Platform Support**: Compatible with various embedded platforms.
- **Easy integration**: Integrate Dmod seamlessly into your existing projects with minimal effort.
- **Lightweight**: Designed to be lightweight and efficient, with minimal impact on system performance.
- **Testing on Host**: Test your modules on a host machine before deploying them to the target platform.
- **Safe deployment**: Update modules without affecting the entire system, ensuring safe and reliable operation.
- **Restarts**: Restart the module without affecting the entire system.

### Use Cases:
- **Firmware Updates**: Apply updates to specific modules without affecting the entire system.
- **Feature Extensions**: Add new features or functionalities on-the-fly.
- **Testing and Debugging**: Load test modules or debugging tools dynamically.
- **Customization**: Customize the behavior of your system based on user input or external conditions.
- **Resource Management**: Manage system resources more efficiently by loading and unloading modules as needed.


By using Dmod, you can achieve greater flexibility and scalability in your embedded systems, ensuring that your applications can adapt to changing requirements and environments.

### Requirements

- **Compiler**: Currently, we only support compilers compatible with GCC.
- **Build System**: We recommend using CMake version 3.18 or higher (older versions have not been tested).
- **Make**: We also support the Make build system, but it must be version 4.2 or newer.
- **Dynamic Memory Allocation**: Your project must support dynamic memory allocation, requiring implementations of `Dmod_Malloc` and `Dmod_Free`.

### Recommended

- **Logging**: We recommend implementing the `Dmod_Printf` function for logging purposes.
- **File System**: If your project supports a file system, you can use it to store and load ***.dmf** files - this will allow for automatic loading of modules' dependencies. 

## What is the Module?

<img src="gimp/graphs/generic_arch.jpg" style="width: 60%;">

A module is a self-contained unit of code that can be dynamically loaded from a `*.dmf` file and unloaded from the system. Modules can be used to **add new features, extend existing functionality**, or customize the behavior of the system **without requiring a full recompilation or restart.** Moreover, they can communicate with each other and the system using a common API. The **Dmod** library manages the loading and unloading of modules, as well as its dependencies, ensuring that the system remains stable and efficient.

Moreover, modules can be **developed and tested independently of the main application**, allowing for easier development, debugging, and sharing across projects. This modular approach makes it easier to manage and extend the system, as well as adapt it to changing requirements and environments.

Thanks to the dependencies management, it is possible and easy to not only load the modules only when they are required, but also **unload not used modules to free up the resources.**

## Communication

The **Dmod** library provides a set of APIs that allow modules to communicate with **each other** and **the system**. These APIs are designed to be simple and easy to use, making it easy to develop modular applications that can be extended and customized as needed.

<img src="gimp/graphs/module-comm.jpg" style="width: 60%;">

What is more, you don't need to provide the source code of the system or a module to use its API - all you need is the **API declaration**, so only the **header file** is required.

### Built-in API

The communication between a module and the system is done through the **builtin API**. The **Dmod** library provides some interface for the modules on its own, but it is also possible and recommended to define your own API as well. You can easily declare any C function as an API function by using the `DMOD_BUILTIN_API` macro:

**Example**:
```c
// YourFunction prototype 
// It can be accessed by name ModuleNameFunctionName
DMOD_BUILTIN_API( ModuleName, 1.0, void, FunctionName, (int arg1, int arg2));
```

The `DMOD_BUILTIN_API` macro takes the following arguments:

- **Module Name**: The name/group of the module that the API function belongs to. (can be empty)
- **Version**: The version of the **function** (not the module) - helps in the future to maintain compatibility.
- **Return Type**: The return type of the function.
- **Function Name**: The name of the function.
- **Arguments**: The arguments of the function.

Your function will be accessible in the system and the modules by the name `<ModuleName><FunctionName>`, so for the example above, it would be `ModuleNameYourFunction`. There is nothing unusual in the usage of the function, so calling it is as simple as calling any other function:

```c
ModuleNameYourFunction(1, 2);
```

To implement the API function, you can either just use the default C function declaration or use the `DMOD_INPUT_API_DECLARATION` macro:

**Example version 1**:
```c
// Implement the API function
void ModuleNameYourFunction(int arg1, int arg2)
{
    // Your code here
}
```

**Example version 2**:
```c
// Usage the DMOD_INPUT_API_DECLARATION macro
// to implement the API function - it can 
// be helpful in the future to maintain
// compatibility (versioning)
DMOD_INPUT_API_DECLARATION(ModuleName, 1.0, void, YourFunction, (int arg1, int arg2))
{
    // Your code here
}
```

The second version of the implementation is recommended, as it allows you to maintain compatibility in the future - if the function signature changes, you can support both versions of the function at the same time.

### Module API

Every module can define its own API that can be used by other modules (or the system). To define the API, you need a special header file that is generated for you by the **Dmod** library in the build process - `<module_name>_defs.h`. This file contains the declarations of macros, that allow you to define the API of your module. Once you include this file in your module, you can use the `dmod_<module_name>_api` macro to define the API functions:

**Example**:
```c
#include "my_module_defs.h"

// Define the API function
// It can be accessed by name my_module_foo
dmod_my_module_api( 1.0, void, _foo, (int arg1, int arg2));
```

The `dmod_<module_name>_api` macro takes the following arguments:

- **Version**: The version of the **function** (not the module) - helps in the future to maintain compatibility.
- **Return Type**: The return type of the function.
- **Function Name**: The name of the function.
- **Arguments**: The arguments of the function.

The rules about the naming are similar to the **built-in API**, so your function will be accessible in the system and the modules by the name `<module_name><FunctionName>`, so for the example above, it would be `my_module_foo`. There is nothing unusual in the usage of the function, so calling it is as simple as calling any other function:

```c
my_module_foo(1, 2);
```
> **Note**: In this example, the function name is prefixed with an underscore `_`, resulting in the actual function name being `_foo`. The underscore is used here for better readability, but it is not required, so you can omit it if you prefer - in this case your full name will be `my_modulefoo`.

To implement the API function, you can either just use the default C function declaration or use the `dmod_<module_name>_api_declaration` macro:

**Example version 1**:
```c
// Implement the API function
void my_module_foo(int arg1, int arg2)
{
    // Your code here
}
```

**Example version 2**:
```c
// Usage the dmod_my_module_api_declaration macro
// to implement the API function - it can
// be helpful in the future to maintain
// compatibility (versioning)
dmod_my_module_api_declaration( 1.0, void, foo, (int arg1, int arg2))
{
    // Your code here
}
```

The second version of the implementation is recommended, as it allows you to maintain compatibility in the future - if the function signature changes, you can support both versions of the function at the same time.

> **⚠️ Warning**: Unlike in the `built-in` API, in the module's API the module name is passed automatically by the `dmod_<module_name>_api` macro and cannot be empty - this is required for the dependency management. However, it is still **possible** to define a **function in the global scope** (check the next chapter).

### Global API

Sometimes it is required for your application to define a function in the *global* scope, so with the name that is **not prefixed** with the **module name**. For example names of functions like `printf` or `malloc` are defined by the standard so we cannot add any prefix to them, however as it was already mentioned, the **Dmod** library requires the name of the module for depenedency management. To solve this problem we introduced the macro `dmod_<module_name>_global_api`, which uses the module name in the dependency management, but does not add it to the function name:

**Example**:
```c
#include "my_module_defs.h"

// Define the API function
// It can be accessed by name foo
dmod_my_module_global_api( 1.0, void, foo, (int arg1, int arg2));
```


The `dmod_<module_name>_global_api` macro takes the following arguments:

- **Version**: The version of the **function** (not the module) - helps in the future to maintain compatibility.
- **Return Type**: The return type of the function.
- **Function Name**: The name of the function.
- **Arguments**: The arguments of the function.

Thanks to this macro, the function will be accessible in the system and the modules by the name `<FunctionName>` (so for the example above, it would be `foo`), however the **Dmod** dependency system will treat it as a part of the `my_module` module.

To implement the API function, you can either just use the default C function declaration or use the `dmod_<module_name>_global_api_declaration` macro:

**Example version 1**:
```c
// Implement the API function
void foo(int arg1, int arg2)
{
    // Your code here
}
```

**Example version 2**:
```c
// Usage the dmod_my_module_global_api_declaration macro
// to implement the API function - it can
// be helpful in the future to maintain
// compatibility (versioning)
dmod_my_module_global_api_declaration( 1.0, void, foo, (int arg1, int arg2))
{
    // Your code here
}
```

The second version of the implementation is recommended, as it allows you to maintain compatibility in the future - if the function signature changes, you can support both versions of the function at the same time.

### Module Abstraction Layer (MAL)

The default approach assumes, that every modules defines it's **API**, that can be consumed by other modules. In this approach, if 
`Module A` requires `Module B`, it needs to have an access to the headers of the `Module B`, include them, call the API functions 
and the **Dmod** library will load the `Module B` as a dependency of the `Module A`. 


<img src="gimp/graphs/default-api.jpg" style="width: 60%;">

However, sometimes we don't want to bind the modules to not force the user to have a dependency to the external repositories
or to be more flexible and make the module compatible with different implementations of submodules. 
In this case, we can use the **Module Abstraction Layer (MAL)**, which allows you to define the output interface of the module, 
without binding it to the specific implementation.

<img src="gimp/graphs/mal-example.jpg" style="width: 60%;">

In other words, we revert a dependency - instead of `Module A` requiring headers of `Module B`, we require the headers 
of the `Module A` in the `Module B`. This way, when the **Dmod** library loads the `Module A`, it searches for the implementation 
of the `Module A MAL` in the available modules or the system and loads it as a dependency of the `Module A`. Thanks to that we can 
decide which implementation we want to use in the runtime - it can be `Module B`, `Module C` or even the system itself.

To define a function as **MAL**, you need to use the `dmod_<module_name>_mal` macro:

**Example**:
```c
#include "my_module_defs.h"

// Define the MAL function
// It can be accessed by name my_module_foo
dmod_my_module_mal( 1.0, void, _foo, (int arg1, int arg2));
```

The `dmod_<module_name>_mal` macro takes the following arguments:

- **Version**: The version of the **function** (not the module) - helps in the future to maintain compatibility.
- **Return Type**: The return type of the function.
- **Function Name**: The name of the function.
- **Arguments**: The arguments of the function.

The rules about the naming are similar to the **built-in API**, so your function will be accessible in the system and the modules by the name `<module_name><FunctionName>`, so for the example above, it would be `my_module_foo`. There is nothing unusual in the usage of the function, so calling it is as simple as calling any other function:

```c
my_module_foo(1, 2);
```

To implement the MAL function in the second module, you need to update your `CMakeLists.txt` or `Makefile` file 
to include the `<module_name>` name in the variable `DMOD_MAL_IMPLS`:

**CMakeList.txt of `Module B`**:
```CMake
# Module B implements the MAL of `my_module`
set(DMOD_MAL_IMPLS
    my_module
)
```

**Makefile of `Module B`**:
```Makefile

# Module B implements the MAL of `my_module`
DMOD_MAL_IMPLS=my_module

```

---

### Dmod Interface (DIF)

While **MAL** allows you to define an interface that can be implemented by **one module at a time** (1:1 relationship), **DIF (Dmod Interface)** enables **multiple modules to implement the same interface simultaneously** (1:N relationship). This is particularly useful for plugin-like architectures where you want to discover and use multiple implementations dynamically.

![DIF Sequence Diagram](docs/images/dif-sequence.png)

The diagram above illustrates the complete DIF workflow: module initialization, DIF registration, runtime discovery with `Dmod_GetNextDifModule()`, and dynamic function calls using multiple filesystem implementations.

**Use Cases for DIF:**
- Multiple filesystem implementations (FAT32, Flash, RAM) used together
- Different hardware drivers (SPI, I2C, UART) available on the same system
- Plugin systems where multiple implementations can be loaded and discovered at runtime

#### Comparison: MAL vs DIF

| Feature | MAL | DIF |
|---------|-----|-----|
| **Implementations** | Single (1:1) | Multiple (1:N) |
| **Dynamic Discovery** | No | Yes |
| **Use Case** | Swappable implementations | Plugin-like architecture |
| **Selection** | Compile/load time | Runtime iteration |
| **Overhead** | Lower | Slightly higher (discovery) |

#### Defining a DIF Interface

Create an interface module that defines the DIF signatures:

**difs.h (Interface Definition)**:
```c
#include "dmod.h"
#include "difs_defs.h"

// Define the DIF with function signatures and typedefs
dmod_difs_dif( 1.0, int, _fopen, (void** fp, const char* path, int mode, int attr) );
dmod_difs_dif( 1.0, int, _fclose, (void* fp) );
```

The `dmod_<module>_dif` macro automatically creates:
- A typedef for the function pointer: `dmod_<module>_<name>_t`
- A signature string accessible via `dmod_<module>_<name>_sig` (used for runtime discovery)

**Important**: Use the `_sig` version of the signature (e.g., `dmod_difs_fopen_sig`) when calling `Dmod_GetNextDifModule()` and `Dmod_GetDifFunction()`. These signatures are automatically generated and exported by the DIF macro.

#### Implementing a DIF

Modules can implement the DIF by using the `dmod_<module>_dif_api_declaration` macro:

**fatfs.c (FatFS Implementation)**:
```c
#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_fatfs
#   define DMOD_fatfs
#endif

#include "dmod.h"
#include "difs.h"

// Implement _fopen for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fopen, (void** fp, const char* path, int mode, int attr) )
{
    // FatFS-specific implementation
    *fp = Dmod_Malloc(32);  // Allocate file handle
    return 0;
}

// Implement _fclose for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fclose, (void* fp) )
{
    Dmod_Free(fp);
    return 0;
}
```

**flashfs.c (FlashFS Implementation)**:
```c
// Similar structure with FlashFS-specific implementation
dmod_difs_dif_api_declaration( 1.0, FlashFS, int, _fopen, (void** fp, const char* path, int mode, int attr) )
{
    // FlashFS-specific implementation
    return 0;
}
```

#### Using DIF - Dynamic Discovery

Modules can discover and use all available DIF implementations at runtime:

**vfs.c (Virtual File System using DIF)**:
```c
#include "dmod.h"
#include "difs.h"

int dmod_init(const Dmod_Config_t *Config)
{
    // Iterate through all modules implementing DIFS
    Dmod_Context_t* fs = Dmod_GetNextDifModule( dmod_difs_fopen_sig, NULL );
    
    while(fs != NULL)
    {
        // Get function pointers from this module
        dmod_difs_fopen_t fopen_func = (dmod_difs_fopen_t)Dmod_GetDifFunction( fs, dmod_difs_fopen_sig );
        dmod_difs_fclose_t fclose_func = (dmod_difs_fclose_t)Dmod_GetDifFunction( fs, dmod_difs_fclose_sig );
        
        // Use the functions
        void* file_handle = NULL;
        fopen_func( &file_handle, "test.txt", 1, 0 );
        // ... work with file ...
        fclose_func( file_handle );
        
        // Get next implementation
        fs = Dmod_GetNextDifModule( dmod_difs_fopen_sig, fs );
    }
    
    return 0;
}
```

#### DIF API Reference

**`Dmod_GetNextDifModule( const char* DifSignature, Dmod_Context_t* Previous )`**

Iterates through all loaded **and enabled** modules that implement a specific DIF function.
- **DifSignature**: The DIF signature string (e.g., `dmod_difs_fopen_sig`)
- **Previous**: Previous module context (NULL to start from the beginning)
- **Returns**: Next module implementing the DIF, or NULL if no more modules

**Important**: Only **enabled** modules are returned by this function. Modules must be both loaded and enabled for their DIF implementations to be discoverable.

**`Dmod_GetDifFunction( Dmod_Context_t* Context, const char* DifSignature )`**

Gets the function pointer for a DIF implementation from a specific module.
- **Context**: Module context returned by `Dmod_GetNextDifModule`
- **DifSignature**: The DIF signature string
- **Returns**: Function pointer, or NULL if not found

#### Module State Requirements

For DIF implementations to be discoverable and usable:

1. **Module must be loaded**: Use `Dmod_LoadModuleByName()` or `Dmod_LoadFile()`
2. **Module must be enabled**: Use `Dmod_EnableModule()` for library modules or `Dmod_StartModule()` for application modules

**Example**:
```c
// Load the DIF implementation module
Dmod_LoadModuleByName("fatfs");

// Enable it (required for DIF discovery!)
Dmod_EnableModule("fatfs", false, NULL);

// Now the module's DIF implementations are discoverable
Dmod_Context_t* fs = Dmod_GetNextDifModule(dmod_difs_fopen_sig, NULL);
```

#### DIF Example Output

```
=== VFS Demo - Using DIF Interfaces ===

Found file system #1
FatFS: Opening file 'test.txt'
  Operations completed successfully

Found file system #2
FlashFS: Opening file 'test.txt'
  Operations completed successfully

Total file systems found: 2
```

For a complete working example, see the [DIF examples directory](examples/dif/).

---
## Getting Started

To use the **Dmod** repository, you need to integrate it into your project first. This section will guide you through the initial steps to get started with Dmod, including integration into your project and developing your first module.

### Integration

To get started with **Dmod**, follow these simple steps to integrate it into your embedded system:

1. **Clone the repository**: Add the source code of this repository into your project in a prefered way. We recommend to add it as a git submodule:

```bash
git submodule add git@github.com:choco-technologies/dmod.git libs/dmod
git submodule update --init --recursive  # updates the dmod repository with it's submodules
```

2. **Copy the configuration file template**: Depending on your build system, copy the [dmod-cfg.cmake](templates/system/dmod-cfg.cmake) or [dmod-cfg.mk](templates/system/dmod-cfg.mk) file into your project and adapt it to your needs. 

2. **Add the library to your build system**: Link the library `dmod` into your project:


**CMake**:
```CMake
# Set the path to your configuration file
set(DMOD_CFG ${CMAKE_SOURCE_DIR}/dmod-cfg.cmake)

# Add the Dmod directory 
add_subdirectory(libs/dmod)

# Link the Dmod library into the target
target_link_libraries(${PROJECT_NAME} dmod)

# Add the Dmod scripts directory to linker paths
target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_SCRIPTS_DIR})
```
    
**Make**:

The `Make` configuration is a little more complicated, because you need to find your linker command and add the `-L ${DMOD_DIR}/scripts` parameter on your own. 

```Makefile
# Set the path to the Dmod library
DMOD_DIR=$(pwd)/libs/dmod 

# Set the path to your configuration file
DMOD_CFG=$(pwd)/dmod-cfg.mk

# Extra rule for building of the dmod library
build_dmod:
  @make -C $(DMOD_DIR) DMOD_CFG="$(DMOD_CFG)"

# #################################################
# 
#   THIS PART DEPENDS ON YOUR PROJECT! 
#
#   You need to add the dmod/scripts directory into
#   your linker command. 
#
#   In GCC you do this by adding:
#                 -L $(DMOD_DIR)/scripts
#   to the command parameters like in the example below 
$(PROJECT_NAME): $(OBJECTS)
	@$(CC) $(CFLAGS) -L $(DMOD_DIR)/scripts -T my_script.ld -o build/app.elf $(OBJECTS)

```

4. **Include `dmod-common.ld` in your linker script**: Find your linker script, and include the `dmod-common.ld` file inside the `.SECTIONS` block:

```ld

SECTIONS
{

    /* Some of your sections */
    ...

    /* Include the dmod definitions */
    INCLUDE dmod-common.ld

    /* More of your sections */
    ...
}
```

5. **Adapt the system API**: The library provides some default implementations of the System Abstract Layer (SAL), which utilize standard libraries such as `stdlib` and `stdio` for resource allocation and logging. These implementations are defined as weak, allowing you to override and customize them to suit your specific needs. 

List of all the API used by the **Dmod** library can be found in the `dmod_sal.h` header, but the **minimum** required API to define includes:

| Function Name        | Description                     | 
| -------------------- | ------------------------------- |
| `Dmod_Malloc`        | *Allocates heap memory*         |
| `Dmod_Free`          | *Releases heap memory*          |
| `Dmod_AlignedAlloc`  | *Allocates heap aligned memory* |


It is also **recommended** (but not mandatory) to define those:

| Function Name        | Description                           | 
| -------------------- | ------------------------------------- |
| `Dmod_Printf`        | *Prints Dmod logs in `printf` format* |
| `Dmod_Mutex_New`     | *Creates new mutex*                   |
| `Dmod_Mutex_Delete`  | *Releases mutex memory*               |
| `Dmod_Mutex_Lock`    | *Locks the mutex*                     |
| `Dmod_Mutex_Unlock`  | *Unlocks the mutex*                   |

6. **Initialize the Dmod system**: Before using any Dmod functions, you must call `Dmod_Initialize()` to initialize the system's global variables. This function should be called once at the beginning of your application, before loading any modules.

```c
#include "dmod.h"

int main(void)
{
    // Initialize Dmod system
    if (!Dmod_Initialize())
    {
        // Handle initialization failure
        return -1;
    }
    
    // Now you can use Dmod functions
    // Load modules, run applications, etc.
    
    // ...
    
    return 0;
}
```

> **⚠️ Important**: Calling `Dmod_Initialize()` is required to ensure that the builtin API sections are properly initialized. While theoretically this might not be necessary in all cases, in practice it prevents linker-related issues and ensures that global variables are properly initialized.


### Module Development

To develop an application or library module, you need to create a **DMF (Dmod Module File)**. This file contains the compiled code of your module, as well as the metadata required for loading and unloading it dynamically.

You can use templates provided in the [templates/module](templates/module/README.md) directory to create your own module. The templates include the necessary files and configurations to get started with developing a module using the **Dmod** library.

#### Automated Module Generation

For convenience, the **Dmod** repository includes a bash script that automates the creation of new modules from templates. This script generates all necessary files (CMakeLists.txt, Makefile, source files, README, and optionally CI/CD pipelines) with minimal user input.

**Quick Start:**
```bash
# Create a simple library module
./scripts/new-module.sh --name my_lib --type library --path ./modules/my_lib

# Create an application module with GitHub Actions workflow
./scripts/new-module.sh --name my_app --type application --path ./modules/my_app --author "Your Name" --github
```

The script supports:
- **Module types**: `library` or `application`
- **Optional parameters**: author name, license, DMOD directory path
- **CI/CD pipelines**: GitHub Actions and Bitbucket pipelines
- **Interfaces**: DIF and MAL interface support for library modules
- **External modules**: Modules outside the DMOD repository tree

For detailed information about the module generator, see [scripts/README.md](scripts/README.md).

#### Hello World Source Code

Here is an example of a simple "Hello World" module that you can use as a starting point:

**main.c**:
```c
#include "dmod.h"

int main(int argc, char *argv[])
{
    // Dmod_Printf is a system API function that prints messages to the console
    Dmod_Printf("Hello, World!\n");
    return 0;
}
```

The `main.c` file contains the main function of the module, which prints the message "Hello, World!" to the console using the `Dmod_Printf` function - this function is a part of the System Abstract Layer (SAL) and should be implemented in your project. 

The DMOD library supports two build systems: **CMake** and **Make**. You can use either of them to build your module.

#### CMake

To build a module using CMake, you need to create a `CMakeLists.txt` file in the module's directory. This file should include the following commands:

```CMake
cmake_minimum_required(VERSION 3.18)

# Set the module name
set(DMOD_MODULE_NAME        my_module)

# Set the module version
set(DMOD_MODULE_VERSION     "0.1")

# Set the module author (it will be displayed in the module information)
set(DMOD_AUTHOR_NAME        "John Doe")

# Set the stack size required by the module and its priority
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

# Add the module executable
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)
```

> **Note**: Please note, that `dmod_add_executable` will create a target for your module, which you can use just like any other CMake target.

Once this file is created, you can build the module using the following commands:

```sh
cmake -B build -S .
cmake --build build/
```


---

#### Make

To build a module using Make, you need to create a `Makefile` in the module's directory. This file should include the following commands:

```Makefile

# Set the module name
DMOD_MODULE_NAME=my_module

# Set the module version
DMOD_MODULE_VERSION=0.1

# Set the module author (it will be displayed in the module information)
DMOD_AUTHOR_NAME=John Doe

# Set the stack size required by the module and its priority
DMOD_STACK_SIZE=1024
DMOD_PRIORITY=0

# Set the module sources
DMOD_CSOURCES=main.c
DMOD_CXXSOURCES=

# Set the module include directories, libraries, and definitions
DMOD_INC_DIRS=../library
DMOD_LIBS=
DMOD_DEFINITIONS=


# Add the module executable
include $(DMOD_DMF_APP_FILE_PATH)

```

Once this file is created, you can build the module using the following commands:

```sh
make
```

Regardless of the build system you choose, the output of the build process will be a DMF file that contains the compiled code of your module and it can be found inside the `build/dmf` directory.

## Building

There are two modes for building the project: MODULE mode and SYSTEM mode.

### MODULE Mode
In MODULE mode, you build dynamic modules that can be later run on embedded architectures. You need to transfer the DMF file with your module to the architecture. This can be done by including it in the flash memory, placing it on an SD card, or any other method, as long as Dmod receives a buffer with the data.

To build in MODULE mode, use the following commands:
```sh
cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
cmake --build build/
```

### SYSTEM Mode
In SYSTEM mode, you build the library when you want to include it in your project that will run dynamic modules. The resulting image is then loaded into the microcontroller's memory, allowing it to execute DMF files.

To build in SYSTEM mode, use the following commands:
```sh
cmake -DDMOD_MODE=DMOD_SYSTEM -B build -S .
cmake --build build/
```

### Installing Tools

Dmod includes utility tools like `todmfc` (DMF Compressor) and `todmp` (DMP Package Creator) that can be installed to your system for easy access.

#### Installing with CMake

To install the tools using CMake, build the project in SYSTEM mode with tools enabled, then use the install target:

```sh
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
cmake --install build/
```

By default, tools are installed to `/usr/local/bin`. You can change the installation prefix:

```sh
cmake --install build/ --prefix /custom/path
```

#### Installing with Make

To install tools using Make, navigate to the tool directory and use the install target:

```sh
cd tools/system/todmfc
make install
```

By default, tools are installed to `/usr/local/bin`. You can customize the installation directory:

```sh
make install INSTALL_PREFIX=/custom/path
```

#### Uninstalling Tools

To uninstall a tool installed with Make:

```sh
cd tools/system/todmfc
make uninstall INSTALL_PREFIX=/custom/path
```

Note: Tools installed with CMake should be uninstalled using your package manager or by manually removing the files from the installation directory.


---

## License


The MIT License (MIT)

Copyright (c) 2024 Patryk Kubiak

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

