file(GLOB SRCS cow/*.cpp cowlite/*.cpp nocow/*.cpp static_core_ctrs_init.cpp dt_ctr_datatypes.cpp)

set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES ${SRCS})
set_property(GLOBAL APPEND PROPERTY GLOBAL_CONTAINERS_SOURCES_REACTOR ${SRCS})




