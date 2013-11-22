set (SRCS tests.cpp)

if (EMIT_LLVM)
    add_executable(tests ${SRCS} $<TARGET_OBJECTS:Memoria> $<TARGET_OBJECTS:MemoriaTools>)
    set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
else()
    if (EMIT_AST)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -emit-ast")
        
        #set (MEMORIA_COMPILE_FLAGS "")
        #include_directories(/usr/include/c++/4.8.1)
        #include_directories(/usr/include/c++/4.8.1/x86_64-redhat-linux)
        #include_directories(/usr/lib/clang/3.3/include)
        #include_directories(/usr/include)
        #set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> -cc1 -ast-print -std=c++11 -fcxx-exceptions <FLAGS> -I/usr/include <SOURCE>")
        
        set(CMAKE_CXX_LINK_EXECUTABLE "cp <OBJECTS> ${LIBRARY_OUTPUT_PATH}/tests.ast")
        
        add_executable(tests ${SRCS})
        set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
        set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    else()    
        add_executable(tests ${SRCS})
        set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_TESTS")
        set_target_properties(tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

        target_link_libraries(tests Memoria MemoriaTools ${MEMORIA_LIBS})
    endif()
endif()