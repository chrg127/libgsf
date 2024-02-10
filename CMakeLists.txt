cmake_minimum_required(VERSION 3.15)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(libgsf VERSION 0.1 LANGUAGES C CXX)

option(BUILD_EXAMPLES "Build provided examples" OFF)
option(BUILD_WITH_ASAN "Build using ASAN" OFF)

include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)
include(CMakePackageConfigHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(libgsf
    SHARED
        src/gsf.cpp
        include/gsf.h
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME gsf
        PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/gsf.h
)

find_package(ZLIB REQUIRED)
add_subdirectory(external)

add_library(libgsf::libgsf ALIAS libgsf)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        # add directory include/ to include directories
        # both at building time and at installation time
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20 c_std_11)
target_compile_definitions(${PROJECT_NAME} PRIVATE GSF_BUILD_SHARED)

if (BUILD_WITH_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()

target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)

set(
    ${PROJECT_NAME}_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    CACHE STRING "Path to install ${PROJECT_NAME} Config*.cmake files to."
)

install(
    TARGETS libgsf
    EXPORT libgsf_targets
    INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT ${PROJECT_NAME}_targets
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
)

if (BUILD_EXAMPLES)
    message("simple example will be built")
    add_executable(example src/example.c)
    if (BUILD_WITH_ASAN)
        target_link_libraries(example asan libgsf)
    else()
        target_link_libraries(example libgsf)
    endif()

    find_package(SDL2)
    if (SDL2_FOUND)
        message("SDL2 example will be built")
        add_executable(example_sdl src/example_sdl.c)
        target_include_directories(example_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_directories(example_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
        if (BUILD_WITH_ASAN)
            target_link_libraries(example_sdl asan ${SDL2_LIBRARIES} SDL2::SDL2main libgsf)
        else()
            target_link_libraries(example_sdl ${SDL2_LIBRARIES} SDL2::SDL2main libgsf)
        endif()
    else()
        message("SDL2 not found, cannot build SDL2 example")
    endif()
endif()
