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

WORK_DIR=.
LIB_DIR=../lib
LIB_NAME="libMemoria.a"
DOXYFILE=Doxyfile
MEMORIA_DIR="../../../memoria/"
RESOURCES_DIR="resources"

if ! [ -e ${WORK_DIR} ]; then
    mkdir ${WORK_DIR};
fi

if ! [ -e "${WORK_DIR}/src" ]; then
    mkdir "${WORK_DIR}/src";
fi

dwarfdump -aid "${LIB_DIR}/${LIB_NAME}" | c++filt | ./mocksrc.py -o "${WORK_DIR}/src/" ${MEMORIA_DIR} -

cd $WORK_DIR
doxygen $DOXYFILE

./postprocess.py html
cp -f "${RESOURCES_DIR}/tabs.css" html
cp -f "${RESOURCES_DIR}/search.js" html/search
cp -f "${RESOURCES_DIR}/search.css" html/search