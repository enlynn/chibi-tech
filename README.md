# chibi-tech

My little toy game engine

# Build

## Dependencies

- CMake
- Ninja
- C/C++ Compiler
    - Clang
    - GCC
- Vulkan
    - [Linux Setup](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started.html)
    - [Installation](https://vulkan.lunarg.com/sdk/home#linux)

On Windows, Clang, CMake, and Ninja can be installed alongside Visual Studio or installed separately. If installed through Visual Studio and running from the command line,
make sure to use Window's Dev Console x64 or run `vcvarsall.bat`. Please note that the MSVC compiler is not supported.

On Linux, see your distro's instruction for installing these packages.

## Building

This project uses CMake with a custom build binary to simplify building and running the engine. To build the buidsystem, run in the root directory of the project:
```
# On Windows using Clang
scripts\setup.bat

# On Linux
chmod +x ./scripts/setup.sh
#   GCC
./scripts/setup.sh gcc
#   Clang
./scripts/setup.sh clang
```

For help using the build tool:
```
ct_build -h
```

which will print:
```
Usage
    ct_build [options]

Options
    -h                      = Display help information
    -r <target>             = Specify an executable to run
    -b <build type>         = Specify a built type
    -t <toolset>            = Specify a toolset supported by the builder system

Targets
    Editor                  = Run the main client engine

Build Type
    Debug                   = Builds with debug symbols enabled and optimizations turned off.
    Release                 = Builds with optimizations turned on.
    RelWithDebInfo          = Builds with debug symbols enabled and optimizations turned on.
    MinSizeRel              = Similar to Release but optimizes for size rather than speed

Toolset
    clang                   = Use the clang compiler
    gcc                     = Use the gcc compiler
```

Example build using clang with a release build:
```
ct_build -b Release -t clang
```

todo: run command
```
ct_build -b Release -r Editor
```

To get compilation database (e.g. clangd):
```
# Build is the build type {debug, rel, ...}
cp bin/$build/compile_commands.json .
```
todo: have the build system do this automagically.

# Features

It, uh, opens a window! Very advanced. Very fancy.
