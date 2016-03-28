#!/bin/sh

## Copyright 2012 Memoria team
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

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
ADDITIONAL_CMAKE_PARAMS=""

if [ $# -eq 0 ] ; then : # do nothig
elif [ $# -eq 1 ]; then
    case $1 in
        --gcc)
            ;; # do nothig
        --clang)
            C_COMPILER="clang"
            CXX_COMPILER="clang++"
            BUILD_DIR="clang"
            ADDITIONAL_CMAKE_PARAMS="-DMEMORIA_LINK_FLAGS=\"-lpthread -L/usr/local/lib\" -DBUILD_TOOLS=true -DMEMORIA_LIBS=libc++.a"
           
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

# escaping ADDITIONAL_CMAKE_PARAMS
if [ -n "$ADDITIONAL_CMAKE_PARAMS" ] ; then
    ADDITIONAL_CMAKE_PARAMS=$(echo "$ADDITIONAL_CMAKE_PARAMS" | sed -e 's/[][\.*^$/]/\\&/g')
fi

BASE_DIR=$(dirname $0)
BUILD_DIR=$BASE_DIR/../../memoria-build/$BUILD_DIR
COMMON_DIR=$BASE_DIR/common

mkdir -p $BUILD_DIR
mkdir -p $BUILD_DIR/bin

cp -i $COMMON_DIR/build.sh.tpl $BUILD_DIR/build.sh
cp -i $COMMON_DIR/clean.sh.tpl $BUILD_DIR/clean.sh

cp -i -r $COMMON_DIR/documentation/ $BUILD_DIR/

sed -e "s/@@C_COMPILER_PLACEHOLDER@@/${C_COMPILER}/" \
    -e "s/@@CXX_COMPILER_PLACEHOLDER@@/${CXX_COMPILER}/" \
    -e "s/@@ADDITIONAL_CMAKE_PARAMS_PLACEHOLDER@@/${ADDITIONAL_CMAKE_PARAMS}/" \
     $COMMON_DIR/mkbuild.sh.tpl > $BUILD_DIR/mkbuild.sh
chmod +x $BUILD_DIR/mkbuild.sh
