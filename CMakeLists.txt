cmake_minimum_required(VERSION 3.21)
project(c4 VERSION 1.0 LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

# The $MODE setting changes compiler options (cmake -DMODE=xyz .):
#   * OPTIMISE - (default) will produce the fastest binary possible.
#   * DEBUG    - runs with assertions and debug symbols enabled.
set(MODE "OPTIMISE" CACHE STRING "Goal for the project build: OPTIMISE, DEBUG")

# Setup targets
file(GLOB_RECURSE SRCS "${CMAKE_SOURCE_DIR}/src/solver/*.cpp")
file(GLOB_RECURSE TSTS "${CMAKE_SOURCE_DIR}/tst/*.cpp")

add_executable(book "${CMAKE_SOURCE_DIR}/src/book.cpp")
add_executable(c4 "${CMAKE_SOURCE_DIR}/src/c4.cpp")
add_executable(play "${CMAKE_SOURCE_DIR}/src/play.cpp")
add_executable(random "${CMAKE_SOURCE_DIR}/src/random.cpp")
add_executable(test ${TSTS})

add_library(solver OBJECT ${SRCS})
target_link_libraries(book PRIVATE solver)
target_link_libraries(c4 PRIVATE solver)
target_link_libraries(play PRIVATE solver)
target_link_libraries(random PRIVATE solver)
target_link_libraries(test PRIVATE solver)

# Add compiler settings depending on target machine and $MODE.
if (MSVC)
    message(STATUS "Compiling with MSVC in ${MODE} mode.")
    
    if (MODE MATCHES "OPTIMISE")
        target_compile_definitions(solver PUBLIC NDEBUG)
        target_compile_options(solver PUBLIC /W4 /Ox /GL)
        target_link_options(solver PUBLIC /GL)
    elseif(MODE MATCHES "DEBUG")
        target_compile_options(solver PUBLIC /W4 /Zi)
    else()
        message(FATAL_ERROR "Mode $MODE not recognised.")
    endif()
else()
    message(STATUS "Compiling with GCC/Clang in ${MODE} mode.")

    # On some systems, clang uses a non standard library which causes compile issues.
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(solver PUBLIC -stdlib=libc++)
        target_link_options(solver PUBLIC -stdlib=libc++)
    endif()
    
    if (MODE MATCHES "OPTIMISE")
        target_compile_definitions(solver PUBLIC NDEBUG)
        target_compile_options(solver PUBLIC -Wall -O3 -flto)
        target_link_options(solver PUBLIC -flto=full)
    elseif(MODE MATCHES "DEBUG")
        target_compile_options(solver PUBLIC -Wall -g)
    else()
        message(FATAL_ERROR "Mode $MODE not recognised.")
    endif()
endif()
