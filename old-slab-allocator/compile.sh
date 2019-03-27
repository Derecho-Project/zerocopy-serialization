#!/bin/bash
if [ ! -d "build" ]; then
  mkdir build
fi
cd build && cmake .. -DCMAKE_CXX_COMPILER=g++-8 -DCMAKE_BUILD_TYPE=Release
