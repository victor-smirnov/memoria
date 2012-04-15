set (SRCS benchmarks.cpp)

add_executable(benchmarks ${SRCS})
set_target_properties(benchmarks PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(benchmarks PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

if (NOT CHECK_SYNTAX_ONLY)
	target_link_libraries(benchmarks Memoria MemoriaTools ${MEMORIA_LIBS})
endif()
