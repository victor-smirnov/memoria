add_library(MemoriaTestsTools STATIC tools.cpp)
set_target_properties(MemoriaTestsTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
set_target_properties(MemoriaTestsTools PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")


if (NOT DEFINED MEMORIA_TESTS)
    set(MEMORIA_TESTS if_then_else idx_map tree_map aliasing array array_ins array_rd vector_ins vector_rd guard)
endif()

FOREACH(MEMORIA_TARGET ${MEMORIA_TESTS})
 	add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    if (NOT CHECK_SYNTAX_ONLY)
        target_link_libraries(${MEMORIA_TARGET} Memoria MemoriaTestsTools ${MEMORIA_LIBS})
    endif()
ENDFOREACH(MEMORIA_TARGET)