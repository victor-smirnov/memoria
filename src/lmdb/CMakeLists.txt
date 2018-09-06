add_library(LMDB STATIC mdb.c midl.c)

target_include_directories(LMDB PUBLIC include)
set_target_properties(LMDB PROPERTIES COMPILE_FLAGS "-pthread -fPIC")
set_target_properties(LMDB PROPERTIES LINK_FLAGS "")
target_link_libraries(LMDB pthread)

set(LMDB_TOOLS mdb_copy mdb_load mdb_dump mdb_stat)

FOREACH(MEMORIA_TARGET_BASE ${LMDB_TOOLS})
    set(MEMORIA_TARGET mma1_${MEMORIA_TARGET_BASE})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET_BASE}.c)
    set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(${MEMORIA_TARGET} LMDB)
ENDFOREACH(MEMORIA_TARGET)
