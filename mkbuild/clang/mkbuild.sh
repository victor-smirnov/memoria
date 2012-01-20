#!/bin/sh

dir="memoria"
if [ "$1" ]
then
  dir="$1"
fi

cmake -G "Unix Makefiles" \
	-DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/usr/bin/clang \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DMEMORIA_LINK_FLAGS="-lpthread -L/usr/local/lib" \
    -DBUILD_TOOLS=true \
    -DMEMORIA_LIBS=libc++.a \
	../../$dir
