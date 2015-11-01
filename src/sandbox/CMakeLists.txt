include_directories(../tools/include)

if (NOT DEFINED MEMORIA_SANDBOX)
    set(MEMORIA_SANDBOX mpl)
endif()

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
#set(BUILD_SHARED_LIBS OFF)


FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX})

    if (EMIT_AST)
        set(CMAKE_CXX_LINK_EXECUTABLE "cp <OBJECTS> ${LIBRARY_OUTPUT_PATH}/${MEMORIA_TARGET}.ast")
        
        add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -emit-ast")
    else()
        
 	    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "-g ${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS} -pthread")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}/sandbox")
        
        #set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_SEARCH_START_STATIC 1)
		#set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_SEARCH_END_STATIC 1)
		#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    
        if (NOT CHECK_SYNTAX_ONLY)
            target_link_libraries(${MEMORIA_TARGET} Memoria MemoriaTools ${MEMORIA_LIBS})
        endif()
    
    endif()
    
ENDFOREACH(MEMORIA_TARGET)