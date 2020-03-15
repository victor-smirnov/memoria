
if (BUILD_CONTAINERS)
    file(GLOB SRCS *_dflt.cpp)

    set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
    set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})

    if (BUILD_MEMORY_STORE_COW OR BUILD_SWMR_STORE_MAPPED)
        file(GLOB SRCS *_cow.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
    endif()

else()
    if(BUILD_CONTAINERS_MULTIMAP)
        file(GLOB SRCS multimap_varchar_varchar_dflt.cpp multimap_uub_dflt.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})

        if (BUILD_MEMORY_STORE_COW OR BUILD_SWMR_STORE_MAPPED)
            file(GLOB SRCS multimap_varchar_varchar_cow.cpp multimap_uub_cow.cpp)

            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
        endif()

    endif()

    if(BUILD_CONTAINERS_SET)
        file(GLOB SRCS set_varchar_dflt.cpp set_uuid_dflt.cpp allocation_map_dflt.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})

        if (BUILD_MEMORY_STORE_COW)
            file(GLOB SRCS set_varchar_cow.cpp set_uuid_cow.cpp allocation_map_cow.cpp)

            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
        endif()
    endif()

    if(BUILD_CONTAINERS_VECTOR)
        file(GLOB SRCS vector_varchar_dflt.cpp vector_uint8_dflt.cpp)

        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
        set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})

        if (BUILD_MEMORY_STORE_COW)
            file(GLOB SRCS vector_varchar_cow.cpp vector_uint8_cow.cpp)

            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})
            set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES_CLASSIC ${SRCS})
        endif()
    endif()
endif()

