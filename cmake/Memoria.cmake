macro(add_global_linker)
   set (MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} ${ARGV}")
endmacro()

macro(add_global_libs)
   set (MEMORIA_LIBS "${MEMORIA_LIBS}" ${ARGV})
endmacro()
