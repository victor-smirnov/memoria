set (SRCS )

set (SRCS ${SRCS} helloworld/hello_world.cpp)

add_executable(tests2 tests2.cpp ${SRCS})
        
set_target_properties(tests2 PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(tests2 PROPERTIES LINK_FLAGS    "${MEMORIA_LINK_FLAGS}")
        
target_link_libraries(tests2 Memoria MemoriaTests2)

target_link_libraries(tests2 ${MEMORIA_LIBS})

