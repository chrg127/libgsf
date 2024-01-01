project(external C CXX)

set(CMAKE_FOLDER external)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/mgba/src/platform/cmake/")

add_subdirectory(mgba/src/arm EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/core EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/gba EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/util EXCLUDE_FROM_ALL)
file(GLOB OS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/platform/posix/*.c)
file(GLOB THIRD_PARTY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/third-party/inih/*.c)
set(CORE_VFS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-fifo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-fd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-dirent.c
)
set(GIT_REV -1)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/core/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/mgba/version.c)
list(APPEND UTIL_SRC ${CMAKE_CURRENT_BINARY_DIR}/mgba/version.c)

target_sources(libgsf
    PRIVATE
        ${ARM_SRC}
        ${GBA_SRC}
        ${CORE_SRC}
        ${UTIL_SRC}
        ${OS_SRC}
        ${CORE_VFS_SRC}
        ${THIRD_PARTY_SRC}
        mgba/src/third-party/blip_buf/blip_buf.c
)

target_compile_definitions(libgsf
    PRIVATE
        HAVE_LOCALE
        MINIMAL_CORE
)

target_include_directories(libgsf
    PRIVATE
        expected/include
        mgba/include
        mgba/src
)
