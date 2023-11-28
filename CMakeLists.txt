cmake_minimum_required(VERSION 3.14)

project(libgsf VERSION 1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# libgsf

## mgba stuff
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/mgba/src/platform/cmake/")
add_subdirectory(external/mgba/src/arm EXCLUDE_FROM_ALL)
add_subdirectory(external/mgba/src/core EXCLUDE_FROM_ALL)
add_subdirectory(external/mgba/src/gba EXCLUDE_FROM_ALL)
add_subdirectory(external/mgba/src/util EXCLUDE_FROM_ALL)
file(GLOB OS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/platform/posix/*.c)
file(GLOB THIRD_PARTY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/third-party/inih/*.c)
set(CORE_VFS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/util/vfs/vfs-mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/util/vfs/vfs-fifo.c
)
list(APPEND CORE_VFS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/util/vfs/vfs-fd.c ${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/util/vfs/vfs-dirent.c)
set(GIT_REV -1)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/external/mgba/src/core/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/external/mgba/version.c)
list(APPEND UTIL_SRC ${CMAKE_CURRENT_BINARY_DIR}/external/mgba/version.c)

find_package(SDL2 REQUIRED)

add_executable(libgsf
    src/libgsf.cpp
    # src/example.c
    src/example_sdl.c
    ${ARM_SRC}
    ${GBA_SRC}
    ${CORE_SRC}
    ${UTIL_SRC}
    ${OS_SRC}
    ${CORE_VFS_SRC}
    ${THIRD_PARTY_SRC}
    external/mgba/src/third-party/blip_buf/blip_buf.c
)

target_compile_definitions(libgsf
    PRIVATE
        HAVE_LOCALE
        MINIMAL_CORE
)

target_include_directories(libgsf
    PRIVATE
        external/expected/include
        external/mgba/include
        external/mgba/src
        ${SDL2_INCLUDE_DIRS}
)

target_link_directories(libgsf
    PRIVATE
        ${SDL2_LIBRARY_DIRS}
)

target_compile_features(libgsf
    PRIVATE
        cxx_std_20
)

target_link_libraries(libgsf
    z
    ${SDL2_LIBRARIES}
)
