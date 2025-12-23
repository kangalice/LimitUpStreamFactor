cmake_minimum_required(VERSION 3.10)
project(FieldGenerator LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(PYBIND11_FINDPYTHON ON)

find_package(pybind11 REQUIRED)
find_package(cxxopts REQUIRED)
find_package(fmtlog REQUIRED)
find_package(internalcpplib REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
add_executable(demo demo/Basic.cpp)

# 复制 config.json 到构建输出目录
# configure_file(config.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.json COPYONLY)

target_include_directories(demo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    ${pybind11_INCLUDE_DIRS}
)

# 查找 MatchEngineImpl 库
find_library(MATCHENGINEIMPL_LIBRARY
    NAMES MatchEngineImpl
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib
    NO_DEFAULT_PATH # 设置不查找标准系统路径, 只查找指定路径
    REQUIRED
)

target_link_libraries(demo
    internalcpplib::internalcpplib
    ${MATCHENGINEIMPL_LIBRARY}
    pybind11::embed
    cxxopts::cxxopts
    fmtlog::fmtlog
)

set_target_properties(demo PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
