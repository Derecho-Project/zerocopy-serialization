cmake_minimum_required(VERSION 3.12)
project(stl_copy_experiment)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_COMPILER c++)

add_compile_options(
  "-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-stdlib=libc++"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-glldb>"
)

add_executable(stl_copy_experiment main.cpp include/SequentialAllocator.h include/FancyPointer.h include/config.h)
target_include_directories(stl_copy_experiment PRIVATE include ../include)

add_executable(fancy_pointer_test main2.cpp include/fancy_pointer.h include/slab_lookup_table)
target_include_directories(fancy_pointer_test PRIVATE include ../include)
