file(GLOB SRCS src/*.cpp)
set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})

add_subdirectory(platforms)
