cmake_minimum_required(VERSION 3.21)
project(c4 VERSION 1.0 LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Compiler settings
if (MSVC)
	# MSVC generates many useless warnings with -Wall.
	add_compile_options(/W4)
else()
	add_compile_options(-Wall)
endif()
include_directories(tracy/public tracy/public/tracy)

# Find source files.
file(GLOB_RECURSE SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE TSTS "${CMAKE_SOURCE_DIR}/tst/*.cpp")

# Tracy client special case
add_library(tracy OBJECT tracy/public/TracyClient.cpp)
target_compile_options(tracy PRIVATE -w -std=c++11 -O3 -flto=full -mcpu=apple-m1 -DTRACY_ENABLE)

# Targets
add_executable(c4 ${SRCS})
target_link_libraries(c4 PRIVATE $<$<BOOL:${profile}>:tracy>)
