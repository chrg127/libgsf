cmake_minimum_required(VERSION 3.14)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(libgsf VERSION 0.1 LANGUAGES C CXX)

option(BUILD_EXAMPLES "Build provided examples" OFF)

include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)
include(CMakePackageConfigHelpers)
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
)

add_subdirectory(external)

add_library(libgsf::libgsf ALIAS libgsf)

message(CMAKE_INSTALL_INCLUDEDIR = ${CMAKE_INSTALL_INCLUDEDIR})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        # add directory include/ to include directories
        # both at building time and at installation time
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME} z)

set(
    ${PROJECT_NAME}_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    CACHE STRING "Path to install ${PROJECT_NAME} Config*.cmake files to."
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_targets
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
    target_link_libraries(example libgsf)

    find_package(SDL2)
    if (SDL2_FOUND)
        message("SDL2 example will be built")
        add_executable(example_sdl src/example_sdl.c)
        target_include_directories(example_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_directories(example_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
        target_link_libraries(example_sdl ${SDL2_LIBRARIES} libgsf)
    else()
        message("SDL2 not found, cannot build SDL2 example")
    endif()
endif()
