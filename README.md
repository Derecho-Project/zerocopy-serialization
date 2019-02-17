# Slab Allocator

## Prerequisites
- g++-8
- cmake

## Cloning the Repository
Run `git clone --recursive https://github.com/Derecho-Project/zerocopy-serialization.git`.
This will get the `google/benchmark` and `Catch2` submodules.

## Compile and Run
After cloning this directory, run `compile.sh`. This will use cmake to generate
a makefile to build any tests and benchmarks. Navigate to the `build` directory
and run `make`. You should be able to run the executables.

## TODO (old)
1. Problem: Should we call `malloc/free` if the size of the object allocated in
the `SlabAllocator.allocate()` should `SingleAllocator`s allowed to be
arbitrarily large?
    - Mostly solved by just increasing the size of SingleAllocator. May cause
    issues with vectors, but for now just assuming it's fine.
2. Lock free stack
3. Make the `all_slabs` lock free so that we can support serialization

## TODO (new)
1. Test having a vector that represents an arena to allocate fixed-size chunks
from. Need to support allocating things that aren't this size. Will probably
have to specialize this to only work with maps/sets/lists for example
