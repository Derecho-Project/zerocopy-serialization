cmake_minimum_required(VERSION 2.8.9)
project(slab_allocator)

# External Libraries
add_subdirectory(lib/Catch2)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
add_subdirectory(lib/benchmark)
include_directories(include lib/benchmark/include)

# Internal Code
string(CONCAT GCC_WARNINGS
    " -Wall -Wextra"
	" -Wshadow"
	" -Wnon-virtual-dtor"
	" -Wcast-align"
	" -Wunused"
	" -Woverloaded-virtual"
	" -Wpedantic"
	" -Wmisleading-indentation"
	" -Wduplicated-cond"
	" -Wduplicated-branches"
	" -Wlogical-op"
	" -Wnull-dereference"
	" -Wuseless-cast"
	" -Wdouble-promotion"
	" -Wformat=2"
    )

SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++17 -O2 -march=native ${GCC_WARNINGS}")
SET(GCC_COVERAGE_LINK_FLAGS    "-lpthread -latomic")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_subdirectory(benchmark_tests)

add_subdirectory(unit_tests)
enable_testing()

#add_executable(single-threaded      "./tests/single-threaded.cpp")
#add_executable(multi-threaded	   "./tests/multi-threaded.cpp")
#
#add_executable(lf-allocator-test "./tests/lf-allocator-test.cpp")
#add_executable(lf-stack "./tests/lf-stack.cpp")
#
#add_executable(pseudo-serialization "./tests/pseudo-serialization.cpp")

add_executable(containers "./tests/containers.cpp")
