#!/bin/sh

print_usage()
{
    SCRIPT_NAME="mkbuild.sh"
    echo "USAGE: $SCRIPT_NAME [--dir DIRECTORY_NAME]"
}

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

BASE_DIR=$(dirname $0)
CURRENT_DIR=`pwd`

# placeholders will be replaced with sed's regexp
C_COMPILER=`which @@C_COMPILER_PLACEHOLDER@@`
CXX_COMPILER=`which @@CXX_COMPILER_PLACEHOLDER@@`

# if you know a better way to specify out-of-source output directory,
# please email me on ivanhoe12@gmail.com <Ivan>
cd $BASE_DIR

cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=$C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
    -DMEMORIA_LINK_FLAGS="-lpthread -L/usr/local/lib" \
    -DBUILD_TOOLS=true \
    -DMEMORIA_LIBS=libc++.a \
    ../../$SOURCE_DIR

cd $CURRENT_DIR