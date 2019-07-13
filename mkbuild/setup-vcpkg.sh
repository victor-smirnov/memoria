#!/bin/bash

mkdir memoria-build
cd memoria-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_TOOLCHAIN_FILE=../vcpkg-memoria/scripts/buildsystems/vcpkg.cmake  -DBUILD_TESTS=ON  ../memoria