<div style="width: 300px;">
  <img src="gimp/logo/Logo-Transparent.png" style="width: 100%;">
</div>

_________


The **Dmod (Dynamic Modules)** library allows you to add the functionality of loading programs and libraries into your **embedded** architecture in runtime mode. This means that you can dynamically extend the capabilities of your embedded system without needing to recompile or restart the entire application. 

### Key Features:
- **Dynamic Loading**: Load and unload modules at runtime.
- **Modular Architecture**: Design your system in a modular way, making it easier to manage and extend.
- **Resource Management**: Efficiently manage resources and dependencies between modules.
- **Cross-Platform Support**: Compatible with various embedded platforms.
- **Easy integration**: Integrate Dmod seamlessly into your existing projects with minimal effort.

### Use Cases:
- **Firmware Updates**: Apply updates to specific modules without affecting the entire system.
- **Feature Extensions**: Add new features or functionalities on-the-fly.
- **Testing and Debugging**: Load test modules or debugging tools dynamically.

By using Dmod, you can achieve greater flexibility and scalability in your embedded systems, ensuring that your applications can adapt to changing requirements and environments.

### Requirements

- **Compiler**: Currently, we only support compilers compatible with GCC.
- **Build System**: We recommend using CMake version 3.18 or higher (older versions have not been tested).
- **Make**: We also support the Make build system, but it must be version 4.2 or newer.
- **Dynamic Memory Allocation**: Your project must support dynamic memory allocation, requiring implementations of `Dmod_Malloc` and `Dmod_Free`.


---
## Getting Started

To use the **Dmod** repository, you need to integrate it into your project first. This section will guide you through the initial steps to get started with Dmod, including integration into your project and developing your first module.

### Integration

To get started with **Dmod**, follow these simple steps to integrate it into your embedded system:

1. **Clone the repository**: Add the source code of this repository into your project in a prefered way. We recommend to add it as a git submodule:

```bash
git submodule add https://bitbucket.org/chocotechnologies/dmod.git libs/dmod
git submodule update --init --recursive  # updates the dmod repository with it's submodules
```

2. **Copy the configuration file template**: Depending on your build system, copy the `dmod-cfg.cmake` or `dmod-cfg.mk` file into your project and adapt it to your needs. 

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


### Module Development



---
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
