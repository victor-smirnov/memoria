#!/bin/sh

dir="memoria"
if [ "$1" ]
then
  dir="$1"
fi

cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../../$dir
