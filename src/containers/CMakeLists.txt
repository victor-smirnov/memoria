
if (BUILD_CONTAINERS)
    file(GLOB SRCS *.cpp)

    set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
    set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
else()
    if(BUILD_CONTAINERS_MULTIMAP)
        file(GLOB SRCS multimap_varchar_varchar_dflt.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
    endif()

    if(BUILD_CONTAINERS_SET)
        file(GLOB SRCS set_varchar_dflt.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
    endif()
endif()

