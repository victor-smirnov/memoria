#!/bin/bash

cd home 
git clone https://vsmirnov@bitbucket.org/vsmirnov/memoria.git
mkdir memoria-build
cd memoria-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_TESTS=ON ../memoria
make -j4