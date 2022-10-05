# CMake configuration for individual programming assignments
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

cmake_minimum_required(VERSION 3.20.0)

project(CMSC237_AUT_2022
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2022/fall/23700-1/index.html"
  DESCRIPTION "individual programming projects for CMSC 23700 (Autumn 2022)"
  LANGUAGES C CXX)

# Disable builds in the source tree to prevent corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
ERROR: builds in the source tree are not allowed.
       Create a separate directory for build files.
")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Add path for custom modules
set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

# find GLM, GLFW, and Vulkan
find_package(glfw 3.3 REQUIRED)
find_package(glm 0.9.9 REQUIRED)
find_package(vulkan 1.3 REQUIRED)

# compiler for shader programs
#
set(GLSLC "${VULKAN_BIN_DIR}/glslangValidator")

option (CS237_ENABLE_DOXYGEN "Enable doxygen for generating cs237 library documentation." OFF)
option (CS237_VERBOSE_MAKEFILE "Enable verbose makefiles." OFF)

# enable verbose makefiles
#
set(CMAKE_VERBOSE_MAKEFILE ${CS237_VERBOSE_MAKEFILE})

# definitions for the configuration header file
#
include(cs237-config)

# Generate the configuration header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/cs237-config.h.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cs237-library/include/cs237-config.h
  @ONLY)

# cs237 library
#
add_subdirectory(cs237-library)
set(CS237_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/cs237-library/include)
set(CS237_DOXYGEN_HTML_DIR ${CMAKE_SOURCE_DIR}/cs237-library/doc)

if (CS237_ENABLE_DOXYGEN)
  message(STATUS "Doxygen enabled.")
  find_package(Doxygen REQUIRED)
  if (DOXYGEN_FOUND)
   # TODO
  endif()
endif()

# the subdirectory for the labs
#
add_subdirectory(labs EXCLUDE_FROM_ALL)

# TODO: subdirectory for individual projects