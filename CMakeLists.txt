cmake_minimum_required(VERSION 3.10)

project(GFXReconstruct-Viewer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Build types" FORCE)

file(GLOB_RECURSE SRC_LIST
    ./src/*.c
    ./src/*.cpp
)

add_executable(GFXReconstruct-Viewer ${SRC_LIST})

target_include_directories(GFXReconstruct-Viewer PRIVATE src)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT GFXReconstruct-Viewer)