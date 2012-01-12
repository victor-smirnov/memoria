#!/bin/sh

dir="memoria"
if [ "$1" ]
then
  dir="$1"
fi

cmake -G "Unix Makefiles" \
	-DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
	../../$dir
