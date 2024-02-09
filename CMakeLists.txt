cmake_minimum_required(VERSION 3.14)
project(c4 VERSION 1.0 LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall)
include_directories(tracy/public tracy/public/tracy)

# Source directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(TST_DIR "${CMAKE_SOURCE_DIR}/tst")
file(GLOB_RECURSE SRCS "${SRC_DIR}/*.cpp")
file(GLOB TSTS "${TST_DIR}/*.cpp")

# Tracy client special case
add_library(tracy OBJECT tracy/public/TracyClient.cpp)
target_compile_options(tracy PRIVATE -w -std=c++11 -O3 -flto=full -mcpu=apple-m1 -DTRACY_ENABLE)

# Targets
add_executable(c4 ${SRCS})
target_link_libraries(c4 PRIVATE $<$<BOOL:${profile}>:tracy>)
