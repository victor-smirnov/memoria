include_directories(../tools/include)

if (NOT DEFINED MEMORIA_SANDBOX)
    set(MEMORIA_SANDBOX instree)
endif()


FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX})

    if (EMIT_AST)
        set(CMAKE_CXX_LINK_EXECUTABLE "cp <OBJECTS> ${LIBRARY_OUTPUT_PATH}/${MEMORIA_TARGET}.ast")
        
        add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -emit-ast")
    else()
    
 	    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}/sandbox")
    
        if (NOT CHECK_SYNTAX_ONLY)
            target_link_libraries(${MEMORIA_TARGET} Memoria MemoriaTools ${MEMORIA_LIBS})
        endif()
    
    endif()
    
ENDFOREACH(MEMORIA_TARGET)