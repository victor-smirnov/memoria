
if (BUILD_CONTAINERS)
    file(GLOB SRCS *.cpp)
elseif(BUILD_CONTAINERS_MULTIIMAP)
    file(GLOB SRCS multimap_varchar_varchar_dflt.cpp)
endif()

set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
