file(GLOB SRCS generated/*.cpp nocow/*_nocow.cpp cow/*_cow.cpp  static_core_ctrs_init.cpp dt_ctr_datatypes.cpp)

set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES ${SRCS})
set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES_REACTOR ${SRCS})

file(GLOB SRCS cowlite/*_cowlite.cpp)

set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES ${SRCS})
set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES_REACTOR ${SRCS})



