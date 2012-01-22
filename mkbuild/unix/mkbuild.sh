#!/bin/sh

print_usage()
{
    SCRIPT_NAME="mkbuild.sh"
    echo "USAGE: $SCRIPT_NAME [--dir DIRECTORY_NAME]"
}

BASE_DIR=$(dirname $0)

if [ $# -eq 0 ] ; then
    SOURCE_DIR="memoria" # default dir
elif [ $# -eq 2 ] ; then
    if [ "$1" != "--dir" ] ; then
        print_usage
        exit 1
    fi
    
    SOURCE_DIR=$2
else
    print_usage
    exit 1
fi

cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=/usr/bin/gcc \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    $BASE_DIR/../../$SOURCE_DIR
