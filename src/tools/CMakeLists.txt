include_directories(include)


file(GLOB TOOLS_LIB_SRC lib/*.cpp)
add_library(MemoriaTools STATIC ${TOOLS_LIB_SRC})
SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(MemoriaTools Memoria)

if (NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS true)
endif()

if (NOT DEFINED BUILD_BENCHMARKS)
    set(BUILD_BENCHMARKS false)
endif()

if (NOT DEFINED BUILD_DUMP)
    set(BUILD_DUMP false)
endif()

if (NOT DEFINED BUILD_EXAMPLES)
    set(BUILD_EXAMPLES false)
endif()

if (BUILD_TESTS) 
    add_subdirectory(tests)
endif()

if (BUILD_BENCHMARKS) 
    add_subdirectory(benchmarks)
endif()

if (BUILD_DUMP)
    add_subdirectory(dump)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()