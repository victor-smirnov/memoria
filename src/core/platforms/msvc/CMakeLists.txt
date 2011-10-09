file(GLOB SRCS ../posix/config.cpp ../posix/container.cpp ../posix/m*.cpp ../posix/s*.cpp ../posix/t*.cpp *.cpp)
set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})

include_directories(../posix)
