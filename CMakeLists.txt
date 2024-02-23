cmake_minimum_required(VERSION 3.21)
project(c4 VERSION 1.0 LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_VERBOSE_MAKEFILE off)

# The $MODE setting changes compiler options (cmake -DMODE=xyz .):
#   * OPTIMISE - (default) will produce the fastest binary possible.
#   * PROFILE  - allows profiling with tracy.
#   * DEBUG    - runs with assertions and debug symbols enabled.
set(MODE "OPTIMISE" CACHE STRING "Goal for the project build: OPTIMISE, PROFILE, DEBUG")

# Setup targets
file(GLOB_RECURSE SRCS "${CMAKE_SOURCE_DIR}/src/solver/*.cpp")
file(GLOB_RECURSE TSTS "${CMAKE_SOURCE_DIR}/tst/*.cpp")

add_executable(c4 "${CMAKE_SOURCE_DIR}/src/c4.cpp")
add_executable(test ${TSTS})

add_library(solver OBJECT ${SRCS})
target_link_libraries(c4 PRIVATE solver)
target_link_libraries(test PRIVATE solver)

# Add compiler settings depending on target machine and $MODE.
if (MSVC)
	message(STATUS "Compiling with MSVC in ${MODE} mode.")
	
	if (MODE MATCHES "OPTIMISE")
		target_compile_definitions(solver PUBLIC NDEBUG)
		target_compile_options(solver PUBLIC /W4 /Ox /GL)
		target_link_options(solver PUBLIC /GL)
	elseif(MODE MATCHES "PROFILE")
		target_compile_definitions(solver PUBLIC NDEBUG TRACY_ENABLE)
		target_compile_options(solver PUBLIC /W4 /Ox /GL)
		target_link_options(solver PUBLIC /GL)
		target_include_directories(solver PUBLIC tracy/public)
		
		target_link_libraries(c4 PRIVATE tracy)
		target_link_libraries(test PRIVATE tracy)
	
		# Tracy must be compiled with C++11.
		target_compile_options(tracy PRIVATE -w -std=c++11 /Ox /GL -DTRACY_ENABLE)
	elseif(MODE MATCHES "DEBUG")
		target_compile_definitions(solver PUBLIC NO_COLOR_OUTPUT)
		target_compile_options(solver PUBLIC /W4 /Zi)
	else()
		message(FATAL_ERROR "Mode $MODE not recognised.")
	endif()

	target_include_directories(solver PUBLIC tracy/public/tracy)
else()
	message(STATUS "Compiling with GCC/Clang in ${MODE} mode.")

	# On some systems, clang uses a non standard library which causes compile issues.
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(solver PUBLIC -stdlib=libc++)
		target_link_options(solver PUBLIC -stdlib=libc++)
	endif()
	
	if (MODE MATCHES "OPTIMISE")
		target_compile_definitions(solver PUBLIC NDEBUG)
		target_compile_options(solver PUBLIC -Wall -O3 -flto=full)
		target_link_options(solver PUBLIC -flto=full)
	elseif(MODE MATCHES "PROFILE")
		target_compile_definitions(solver PUBLIC NDEBUG TRACY_ENABLE)
		target_compile_options(solver PUBLIC -Wall -O3 -flto=full)
		target_link_options(solver PUBLIC -flto=full)
		target_include_directories(solver PUBLIC tracy/public)

		# Tracy must be compiled with C++11.
		add_library(tracy OBJECT tracy/public/TracyClient.cpp)
		target_compile_options(tracy PRIVATE -w -std=c++11 -O3 -flto=full -DTRACY_ENABLE)
		
		target_link_libraries(c4 PRIVATE tracy)
		target_link_libraries(test PRIVATE tracy)
	elseif(MODE MATCHES "DEBUG")
		target_compile_definitions(solver PUBLIC NO_COLOR_OUTPUT)
		target_compile_options(solver PUBLIC -Wall -g)
	else()
		message(FATAL_ERROR "Mode $MODE not recognised.")
	endif()

	target_include_directories(solver PUBLIC tracy/public/tracy)
endif()
