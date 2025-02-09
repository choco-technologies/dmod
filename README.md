# <img src="gimp/logo/Logo-Transparent.png"></img> 
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
