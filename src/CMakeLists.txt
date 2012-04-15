include_directories(../include)
include_directories(core/include)

if (CLANG_COMPILER)
	include_directories(/usr/local/include/c++/v1)
endif()

add_subdirectory(core)
add_subdirectory(allocators)

get_property(GLOBAL_SRCS GLOBAL PROPERTY GLOBAL_SOURCES)

if (NOT DEFINED MEMORIA_PROFILE)
    set(MEMORIA_PROFILE false)
endif()

if (MEMORIA_PROFILE)
    set(MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -g -pg")
    set(MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} -pg")
endif()


add_library(Memoria STATIC ${GLOBAL_SRCS})
SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
SET_TARGET_PROPERTIES(Memoria PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")


if (NOT CHECK_SYNTAX_ONLY)
    export(TARGETS Memoria FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)
endif()

if (NOT DEFINED BUILD_TOOLS)
    set(BUILD_TOOLS true)
endif()

if (NOT DEFINED BUILD_SANDBOX)
    set(BUILD_SANDBOX false)
endif()

if (BUILD_TOOLS) 
    add_subdirectory(tools)
endif()

if (BUILD_SANDBOX) 
    add_subdirectory(sandbox)
endif()