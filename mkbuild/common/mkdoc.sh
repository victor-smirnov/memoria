#!/bin/sh

WORK_DIR=documentation
LIB_DIR=lib
LIB_NAME="libMemoria.a"

if ! [ -e ${WORK_DIR} ]; then
    mkdir ${WORK_DIR};
fi

# -aid grep DW_TAG_subprogram | grep DW_AT_MIPS_linkage_name
dwarfdump -aid "${LIB_DIR}/${LIB_NAME}" | c++filt | ./mocksrc.py -o ${WORK_DIR}