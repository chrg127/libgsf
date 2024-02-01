# libgsf

libgsf is a library for reading and playing GSF (GBA Sound File) and MiniGSF
files. At the time of writing, it supports only the very simple stuff:

- Reading files, with library support
- Playing samples
- Reading tags

Note that the library is currently in beta. While it's already usable, you may
find that by upgrading some stuff might break.

# Compiling

Compiling is done through CMake. Use the following commands to build:

    mkdir build
    cd build
    cmake .. -DBUILD_EXAMPLES=ON
    make

This will build both the library and the two examples provided. You will find
them inside the `build/` directory.

## Windows

On Windows, life is more complicated. The recommended way is using conan (only
used to install zlib). Issue the following commands:

    conan install . --output-folder=build --build=missing
    cd build
    cmake .. -DBUILD_EXAMPLES=ON -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . --config Release

# Installing

While still inside the `build/` directory:

    make install

# Using

If the library has been built and installed using the commands above, then
you'll be able to link to it using `-lgsf`. When using CMake, you can also
use `find_package(libgsf REQUIRE)` to include it in your project (link it with
name `libgsf`).
Otherwise, if using CMake, you can add this repository as a subdirectory, then
use `add_subdirectory(libgsf)` to include in your project.
