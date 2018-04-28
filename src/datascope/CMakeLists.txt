
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(TARGET_NAME datascope)

#set(DATASCOPE_CTRS ../tests-core2/tests/prototype/btss/btss_test_ctr_dflt.cpp)
#set(DATASCOPE_CTRS ../tests-core2/tests/prototype/btfl/btfl_test_ctr_dflt.cpp)
#set(DATASCOPE_CTRS ../tests-core2/tests/map/map_test_ctr_uuid_long_dflt.cpp)
set(DATASCOPE_CTRS ../tests-core2/tests/multimap/multimap_dbl_i64_dflt.cpp)
set(DATASCOPE_CTRS ../tests-core2/tests/multimap/multimap_i64_i64_dflt.cpp)
set(DATASCOPE_CTRS ../tests-core2/tests/multimap/multimap_string_i64_dflt.cpp)

add_executable(${TARGET_NAME} datascope.cpp main_window.cpp vertex_tree_item.cpp allocator_model.cpp main_window.ui datascope.qrc ${DATASCOPE_CTRS})
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(${TARGET_NAME} Memoria ${MEMORIA_LIBS} ${Qt5_LIBRARIES})

qt5_use_modules(${TARGET_NAME} Widgets Core Gui)
