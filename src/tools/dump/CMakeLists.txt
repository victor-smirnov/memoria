file(GLOB SRCS *.cpp)


add_library(MemoriaToolsTools STATIC tools.cpp)
SET_TARGET_PROPERTIES(MemoriaToolsTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
set_target_properties(MemoriaToolsTools PROPERTIES COMPILE_FLAGS "-DMEMORIA_STATIC")
set_target_properties(MemoriaToolsTools PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")


add_executable(dump ${SRCS} ${INCS})
SET_TARGET_PROPERTIES(dump PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
SET_TARGET_PROPERTIES(dump PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

if (NOT CHECK_SYNTAX_ONLY)
    target_link_libraries(dump Memoria MemoriaToolsTools)
endif()
    