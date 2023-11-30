cmake_minimum_required(VERSION 3.14)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(gsf
    VERSION 0.1
    LANGUAGES C CXX
)

include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)
include(CMakePackageConfigHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(gsf
    SHARED
        src/gsf.cpp
        include/gsf.h
)

add_subdirectory(external)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HAVE_LOCALE
        MINIMAL_CORE
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        external/expected/include
        external/mgba/include
        external/mgba/src
    PUBLIC
        # Due to the way installation work, we only want this path set when building,
        # not once installed
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_compile_features(${PROJECT_NAME}
    PRIVATE
        cxx_std_20
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/gsf.h
)

target_link_libraries(${PROJECT_NAME} z)

install(
    TARGETS
        ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install(
#     EXPORT ${PROJECT_NAME}_Targets
#     NAMESPACE ${PROJECT_NAME}::
#     # FILE ${PROJECT_NAME}Targets.cmake
#     DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
# )

# install(
#     FILES ${CMAKE_CURRENT_LIST_DIR}/include/gsf.h
    # ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    # ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    # DESTINATION include
# )

option(${PROJECT_NAME}_BUILD_EXAMPLES "Build provided examples" OFF)

if (${PROJECT_NAME}_BUILD_EXAMPLES)
    message("simple example will be built")
    add_executable(example src/example.c)
    target_link_libraries(example gsf)

    find_package(SDL2 REQUIRED)
    if (SDL2_FOUND)
        message("SDL2 example will be built")
        add_executable(example_sdl src/example_sdl.c)
        target_include_directories(example_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_directories(example_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
        target_link_libraries(example_sdl ${SDL2_LIBRARIES} gsf)
    else()
        message("SDL2 not found, cannot build SDL2 example")
    endif()
endif()
