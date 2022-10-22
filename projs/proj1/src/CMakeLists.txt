# CMake configuration for individual project 1 sources
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

set(TARGET proj1)

set(SRCS
  app.cpp
  main.cpp
  mesh.cpp
  scene.cpp
  window.cpp)

# path to CS237 Library include files
include_directories(${CS237_INCLUDE_DIR})

add_executable(${TARGET} ${SRCS})

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} proj1-shaders)
