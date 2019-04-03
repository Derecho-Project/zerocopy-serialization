if [ $# -eq 0 ]; then
  echo "Specify one of 'Debug' or 'Release'.\nusage: ./compile.sh <Debug|Release>"
  exit 1
fi

mkdir -p build

cd build

build_type=$1

if [ $# -le 1 ]; then
  CXX='/usr/local/opt/llvm/bin/clang++' cmake .. -DCMAKE_BUILD_TYPE=$build_type
  make
else
  CXX='cc_args.py /usr/local/opt/llvm/bin/clang++' cmake .. -DCMAKE_BUILD_TYPE=$build_type
  make
  find . | grep "clang_complete" | xargs cat | sort | uniq > ../.clang_complete
fi
