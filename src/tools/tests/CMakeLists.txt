set (SRCS tests.cpp cling_tests.cpp)

add_executable(tests ${SRCS})
set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

if (NOT CHECK_SYNTAX_ONLY)
	target_link_libraries(tests Memoria MemoriaTools ${MEMORIA_LIBS})
endif()
