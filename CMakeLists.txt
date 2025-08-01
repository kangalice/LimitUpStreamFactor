cmake_minimum_required(VERSION 3.10)
project(base_factor_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")
set(PYBIND11_FINDPYTHON ON)

find_package(pybind11 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${pybind11_INCLUDE_DIRS})
link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(demo demo/Basic.cpp)

target_link_libraries(demo MatchEngineImpl pybind11::embed)
