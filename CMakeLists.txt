cmake_minimum_required(VERSION 3.6)
project(playgsf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(libresample-0.1.3/include)

add_executable(playgsf
    src/gsf.cpp
    src/linuxmain.cpp
    src/loadpic.cpp
    src/main.c

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
