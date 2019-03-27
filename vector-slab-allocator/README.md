# Vector Slab Allocator

## Running the code
I've been testing using the Homebrew version of clang (version 7.0.1).

First, run `mkdir build` to create a directory where you will build everything.
Then, `cd build` and run `$ cmake .. -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_BUILD_TYPE=Release`
Then, run `make` to compile all the tests.
Finally, you can run some executables.

