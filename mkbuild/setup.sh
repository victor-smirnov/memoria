#!/bin/sh

print_usage()
{
    SCRIPT_NAME="setup.sh"
    echo "USAGE: $SCRIPT_NAME [--gcc|--clang]"
    echo "Default is --gcc"
}

# default is gcc
C_COMPILER="gcc"
CXX_COMPILER="g++"
BUILD_DIR="unix"

if [ $# -eq 0 ] ; then : # do nothig
elif [ $# -eq 1 ]; then
    case $1 in
        --gcc)
            ;; # do nothig
        --clang)
            C_COMPILER="clang"
            CXX_COMPILER="clang++"
            BUILD_DIR="clang"
            ;;
        *)
            print_usage
            exit 1
            ;;
    esac
else
    print_usage
    exit 1
fi

BASE_DIR=$(dirname $0)
BUILD_DIR=$BASE_DIR/../../memoria-build/$BUILD_DIR
COMMON_DIR=$BASE_DIR/common

mkdir -p $BUILD_DIR

cp -i $COMMON_DIR/build.sh.tpl $BUILD_DIR/build.sh
cp -i $COMMON_DIR/clean.sh.tpl $BUILD_DIR/clean.sh

sed -e "s/@@C_COMPILER_PLACEHOLDER@@/${C_COMPILER}/" \
    -e "s/@@CXX_COMPILER_PLACEHOLDER@@/${CXX_COMPILER}/" \
     $COMMON_DIR/mkbuild.sh.tpl > $BUILD_DIR/mkbuild.sh
chmod +x $BUILD_DIR/mkbuild.sh
