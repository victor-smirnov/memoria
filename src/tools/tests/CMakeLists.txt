set (SRCS tests.cpp)

add_executable(tests ${SRCS})
        
set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_TESTS")
set_target_properties(tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        
target_link_libraries(tests Memoria MemoriaTools ${MEMORIA_LIBS})

if (BUILD_MSVC)
    
else()
    target_link_libraries(tests boost_system boost_filesystem)
endif()

