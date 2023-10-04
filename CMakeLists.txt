cmake_minimum_required(VERSION 3.14)

project(playgsf VERSION 1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)

# add_subdirectory(external EXCLUDE_FROM_ALL)
add_subdirectory(external/libresample-cmake EXCLUDE_FROM_ALL)

add_executable(playgsf
    src/linuxmain.cpp
    src/VBA/bios.cpp
    src/VBA/GBA.cpp
    src/VBA/getopt1.c
    src/VBA/Globals.cpp
    src/VBA/memgzio.c
    src/VBA/psftag.c
    src/VBA/snd_interp.cpp
    src/VBA/Sound.cpp
    src/VBA/unzip.cpp
    src/VBA/Util.cpp
)

target_compile_features(playgsf PRIVATE cxx_std_20)

# target_include_directories(playgsf PRIVATE external/libresample/include)

target_compile_definitions(playgsf
    PRIVATE
        LINUX
        C_CORE
)

target_link_libraries(playgsf
    PRIVATE
        z   # zlib
        ao
        resample
        # ${CMAKE_SOURCE_DIR}/external/libresample/lib/libresample.so
)
