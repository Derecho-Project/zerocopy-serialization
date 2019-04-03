cmake_minimum_required(VERSION 3.12)
project(vector_slab_allocator)

add_compile_options(
    "-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-stdlib=libc++"
    "-std=c++17" "$<$<CONFIG:DEBUG>:-O0;-g3;-glldb>"
    )

add_executable(test1 tests/test1.cpp)
target_include_directories(test1 PRIVATE include)

add_executable(test2 tests/test2.cpp)
target_include_directories(test2 PRIVATE include)

add_executable(test3 tests/test3.cpp)
target_include_directories(test3 PRIVATE include)

add_executable(test4 tests/test4.cpp)
target_include_directories(test4 PRIVATE include)

add_executable(test_containers tests/test_containers.cpp)
target_include_directories(test_containers PRIVATE include)

