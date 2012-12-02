#examples.cpp 
set (SRCS examples.cpp)

add_executable(examples ${SRCS})
set_target_properties(examples PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(examples PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

if (NOT CHECK_SYNTAX_ONLY)
	target_link_libraries(examples Memoria MemoriaTools ${MEMORIA_LIBS})
endif()
