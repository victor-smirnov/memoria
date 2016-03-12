include_directories(include)

file(GLOB TOOLS_LIB_SRC lib/*.cpp)

if (EMIT_LLVM)
    add_library(MemoriaTools OBJECT ${TOOLS_LIB_SRC})
    SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN")
else()
    add_library(MemoriaTools STATIC ${TOOLS_LIB_SRC})
    SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN")
    target_link_libraries(MemoriaTools Memoria)
endif()


if (NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS true)
endif()

if (NOT DEFINED BUILD_BENCHMARKS)
    set(BUILD_BENCHMARKS false)
endif()

if (NOT DEFINED BUILD_EXAMPLES)
    set(BUILD_EXAMPLES true)
endif()

if (BUILD_TESTS) 
    add_subdirectory(tests)
endif()

if (BUILD_BENCHMARKS) 
    add_subdirectory(benchmarks)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()