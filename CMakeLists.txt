cmake_minimum_required(VERSION 3.14)

project(playgsf VERSION 1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)

# add_subdirectory(external EXCLUDE_FROM_ALL)
add_subdirectory(external/libresample-cmake EXCLUDE_FROM_ALL)

add_executable(playgsf
    playgsf-src/linuxmain.cpp
    playgsf-src/VBA/bios.cpp
    playgsf-src/VBA/GBA.cpp
    playgsf-src/VBA/getopt1.c
    playgsf-src/VBA/Globals.cpp
    playgsf-src/VBA/memgzio.c
    playgsf-src/VBA/psftag.c
    playgsf-src/VBA/snd_interp.cpp
    playgsf-src/VBA/Sound.cpp
    playgsf-src/VBA/unzip.cpp
    playgsf-src/VBA/Util.cpp
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


include_directories(external/expected/include)
add_executable(libgsf src/libgsf.cpp src/example.c)
target_compile_features(libgsf PRIVATE cxx_std_20)
target_link_libraries(libgsf z)
