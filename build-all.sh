#!/bin/sh

# mkdir -p build_release 
# cd build_release
# conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cmake --build .
# cd ..

mkdir -p build_debug
cd build_debug
conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cd ..

