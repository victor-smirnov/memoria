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
ADDITIONAL_CMAKE_PARAMS='@@ADDITIONAL_CMAKE_PARAMS_PLACEHOLDER@@'

# if you know a better way to specify out-of-source output directory,
# please email me on ivanhoe12@gmail.com <Ivan>
cd $BASE_DIR

cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=$C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBOOST_ROOT=/usr/include \    
    $ADDITIONAL_CMAKE_PARAMS \
    ../../$SOURCE_DIR

cd $CURRENT_DIR
