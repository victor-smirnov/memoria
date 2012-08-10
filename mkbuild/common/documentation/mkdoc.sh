#!/bin/sh

WORK_DIR=.
LIB_DIR=../lib
LIB_NAME="libMemoria.a"
DOXYFILE=Doxyfile

if ! [ -e ${WORK_DIR} ]; then
    mkdir ${WORK_DIR};
fi

if ! [ -e "${WORK_DIR}/src" ]; then
    mkdir "${WORK_DIR}/src";
fi

dwarfdump -aid "${LIB_DIR}/${LIB_NAME}" | c++filt | ./mocksrc.py -o "${WORK_DIR}/src/"

cd $WORK_DIR
doxygen $DOXYFILE

./postprocess.py html
cp -f tabs.css html
cp -f search.js html/search
cp -f search.css html/search