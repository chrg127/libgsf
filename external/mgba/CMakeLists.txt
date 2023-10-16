project(mgba C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/src/platform/cmake/")

add_subdirectory(src/gba)

# message("gba-src = " ${GBA_SRC})

add_library(mgba SHARED ${GBA_SRC})
target_include_directories(mgba PUBLIC include)
