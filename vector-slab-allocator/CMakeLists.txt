cmake_minimum_required(VERSION 3.12)
project(vector_slab_allocator)

add_compile_options(
    "-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-stdlib=libc++"
    "-std=c++17" "$<$<CONFIG:DEBUG>:-O0;-g3;-glldb>"
    )

# Tests for just the slab
add_executable(test1 tests/test1.cpp)
target_include_directories(test1 PRIVATE include)

add_executable(test2 tests/test2.cpp)
target_include_directories(test2 PRIVATE include)

add_executable(test3 tests/test3.cpp)
target_include_directories(test3 PRIVATE include)

add_executable(test4 tests/test4.cpp)
target_include_directories(test4 PRIVATE include)

# Tests for slab with fancy pointers
add_executable(test_integration1 tests/test_integration1.cpp)
target_include_directories(test_integration1 PRIVATE include)

add_executable(test_integration2 tests/test_integration2.cpp)
target_include_directories(test_integration2 PRIVATE include)

add_executable(test_integration3 tests/test_integration3.cpp)
target_include_directories(test_integration3 PRIVATE include)

add_executable(test_integration4 tests/test_integration4.cpp)
target_include_directories(test_integration4 PRIVATE include)

# Tests for slab allocator
add_executable(test_allocator1 tests/test_allocator1.cpp)
target_include_directories(test_allocator1 PRIVATE include)

add_executable(test_allocator2 tests/test_allocator2.cpp)
target_include_directories(test_allocator2 PRIVATE include)

add_executable(test_allocator3 tests/test_allocator3.cpp)
target_include_directories(test_allocator3 PRIVATE include)

add_executable(test_allocator4 tests/test_allocator4.cpp)
target_include_directories(test_allocator4 PRIVATE include)

# Test containers with the slab allocator
add_executable(test_containers tests/test_containers.cpp)
target_include_directories(test_containers PRIVATE include)

