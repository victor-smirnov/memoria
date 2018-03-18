
set(CMAKE_AUTOMOC ON)

set(TARGET_NAME datascope)


add_executable(${TARGET_NAME} datascope.cpp)
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(${TARGET_NAME} Memoria ${MEMORIA_LIBS} ${Qt5_LIBRARIES})
qt5_use_modules(${TARGET_NAME} Widgets Core Gui)