
#!/bin/sh

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