cmake_minimum_required(VERSION 3.6)
project(playgsf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(external/libresample/include)

add_compile_definitions(LINUX)
add_compile_definitions(C_CORE)

add_executable(playgsf
    src/gsf.cpp
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

target_link_libraries(playgsf z ao ${CMAKE_SOURCE_DIR}/external/libresample/lib/libresample.a)
