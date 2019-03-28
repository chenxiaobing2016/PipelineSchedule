cmake_minimum_required(VERSION 2.8)
project(PipelineSchedule)

set(CMAKE_NM "/usr/bin/gcc-nm")
set(CMAKE_AR "/usr/bin/gcc-ar")
set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wreturn-type -Wparentheses")

include_directories("include")
file(GLOB_RECURSE SRC src/*.cpp)

add_executable(pplsche main.cpp ${SRC})
