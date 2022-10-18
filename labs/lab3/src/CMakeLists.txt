# CMake configuration for lab assignment 3 sources
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

set(TARGET lab3)

set(SRCS
  main.cpp
  mesh-data.cpp)

# path to CS237 Library include files
include_directories(${CS237_INCLUDE_DIR})

add_executable(${TARGET} ${SRCS})

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} lab3-shaders)
