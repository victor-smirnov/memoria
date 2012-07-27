#!/bin/sh

WORK_DIR=documentation
LIB_DIR=lib
LIB_NAME="libMemoria.a"
DOXYFILE=Doxyfile

if ! [ -e ${WORK_DIR} ]; then
    mkdir ${WORK_DIR};
fi

dwarfdump -aid "${LIB_DIR}/${LIB_NAME}" | c++filt | ./mocksrc.py -o ${WORK_DIR}

cd $WORK_DIR
doxygen ../$DOXYFILE
cd ..