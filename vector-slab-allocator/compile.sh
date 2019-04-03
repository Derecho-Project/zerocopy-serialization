mkdir -p build

cd build

if [ $# -eq 0 ]; then
  CXX='/usr/local/opt/llvm/bin/clang++' cmake .. -DCMAKE_BUILD_TYPE=Release
  make
else
  CXX='cc_args.py /usr/local/opt/llvm/bin/clang++' cmake .. -DCMAKE_BUILD_TYPE=Release
  make
  find . | grep "clang_complete" | xargs cat | sort | uniq > ../.clang_complete
fi
