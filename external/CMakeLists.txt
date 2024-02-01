project(external C CXX)

include(CheckFunctionExists)
include(CheckIncludeFiles)

set(CMAKE_FOLDER external)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/mgba/src/platform/cmake/")

add_subdirectory(mgba/src/arm EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/core EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/gba EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/util EXCLUDE_FROM_ALL)
add_subdirectory(mgba/src/feature EXCLUDE_FROM_ALL)

set(CORE_VFS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-fifo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/util/vfs/vfs-fd.c
)

if (WIN32)
    file(GLOB OS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/platform/windows/*.c)
	add_definitions(-D_WIN32_WINNT=0x0600)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
	list(APPEND CORE_VFS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/platform/windows/vfs-w32.c)
	if(MSVC)
        target_compile_definitions(libgsf PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _UNICODE UNICODE)
    else()
        target_compile_definitions(libgsf PRIVATE _GNU_SOURCE)
    endif()
    target_link_libraries(libgsf ws2_32 shlwapi)
elseif(UNIX)
    file(GLOB OS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/platform/posix/*.c)
	list(APPEND CORE_VFS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/util/vfs/vfs-dirent.c)
endif()

file(GLOB THIRD_PARTY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mgba/src/third-party/inih/*.c)
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

target_compile_definitions(libgsf PRIVATE MINIMAL_CORE MGBA_DLL DISABLE_THREADING)

check_function_exists(newlocale, HAVE_NEWLOCALE)
check_function_exists(freelocale, HAVE_FREELOCALE)
check_include_files("xlocale.h" HAVE_XLOCALE)
check_function_exists(strdup, HAVE_STRDUP)

if(HAVE_NEWLOCALE AND HAVE_FREELOCALE OR APPLE)
    target_compile_definitions(libgsf PRIVATE HAVE_LOCALE)
endif()
if (HAVE_XLOCALE)
    target_compile_definitions(libgsf PRIVATE HAVE_XLOCALE)
endif()
#if (HAVE_STRDUP)
target_compile_definitions(libgsf PRIVATE HAVE_STRDUP)
#endif()

target_include_directories(libgsf
    PRIVATE
        expected/include
        mgba/include
        mgba/src
)