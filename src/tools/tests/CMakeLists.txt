set (SRCS tests.cpp)

if (EMIT_LLVM)
    add_executable(tests ${SRCS} $<TARGET_OBJECTS:Memoria> $<TARGET_OBJECTS:MemoriaTools>)
    set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
else()
    if (EMIT_AST)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -emit-ast")
  
        set(CMAKE_CXX_LINK_EXECUTABLE "cp <OBJECTS> ${LIBRARY_OUTPUT_PATH}/tests.ast")
        
        add_executable(tests ${SRCS})
        set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    else()
#       add_executable(type_tests type_tests.cpp)
#       set_target_properties(type_tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_TESTS")
#       set_target_properties(type_tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
#       target_link_libraries(type_tests Memoria MemoriaTools ${MEMORIA_LIBS})
    
        add_executable(tests ${SRCS})
        set_target_properties(tests PROPERTIES COMPILE_FLAGS "-g ${MEMORIA_COMPILE_FLAGS} -DMEMORIA_TESTS -DMEMORIA_STATIC")
        set_target_properties(tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS} -luuid")
        
        target_link_libraries(tests Memoria MemoriaTools ${MEMORIA_LIBS})
    endif()
endif()