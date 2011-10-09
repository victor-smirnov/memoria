if (NOT DEFINED MEMORIA_TOOLS)
    set (MEMORIA_TOOLS dump)
endif()

foreach(TARGET ${MEMORIA_TOOLS})
    add_subdirectory(${TARGET})
endforeach(TARGET)
