# libgsf

libgsf is a library for reading and playing GSF (GBA Sound File) and MiniGSF
files. At the time of writing, it supports only the very simple stuff:

- Reading files
- Playing samples
- Seeking
- Reading tags and other info

Note that the library is currently in beta. While it's already usable, you may
find that by upgrading some stuff might break.

# Using

You can either compile and install it manually, or add this repository inside
your project and use CMake's `add_subdirectory` to get it compiled
automatically.

# Building

## Dependencies

Required dependencies are zlib and mgba. You can install zlib through your
package manager, while mgba is included inside the repo.

## Compiling

For starters, clone this repository with

    git clone --recursive https://github.com/chrg127/libgsf

Compiling is done through CMake. Use the following commands to build:

    mkdir build
    cd build
    cmake .. -DBUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release

This will build both the library and the two examples provided inside the
directory `build`.
You can then install through this command:

    cmake --install . --config Release --prefix /path/to/installation

## Windows

On Windows, life is more complicated. The recommended way is using conan (only
used to install zlib (and SDL for the examples)). Issue the following commands:

    conan install . --output-folder=build --build=missing
    cd build
    cmake .. -DBUILD_EXAMPLES=ON -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . --config Release

For installing, it's the same as above.
