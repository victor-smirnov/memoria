#!/bin/sh

print_usage()
{
    SCRIPT_NAME="mkbuild.sh"
    echo "USAGE: $SCRIPT_NAME [--dir DIRECTORY_NAME]"
}

if [ $# -eq 0 ] ; then
    DIR="memoria" # default dir
elif [ $# -eq 2 ] ; then
    if [ "$1" != "--dir" ] ; then
        print_usage
        exit 1
    fi
    
    DIR=$2
else
    print_usage
    exit 1
fi

cmake -G "Unix Makefiles" \
	-DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/usr/bin/clang \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DMEMORIA_LINK_FLAGS="-lpthread -L/usr/local/lib" \
    -DBUILD_TOOLS=true \
    -DMEMORIA_LIBS=libc++.a \
	../../$DIR
