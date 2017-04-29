set (SRCS tests.cpp)

set (SRCS ${SRCS} bitmap/bitmap_test_suite.cpp)
set (SRCS ${SRCS} packed/codecs/packed_codecs_test_suite.cpp)
set (SRCS ${SRCS} packed/allocator/palloc_test_suite.cpp)
set (SRCS ${SRCS} packed/maxtree/packed_maxtree_test_suite.cpp)
set (SRCS ${SRCS} packed/sequence/rle/rleseq_test_suite.cpp)

set (SRCS ${SRCS} map/map_test_suite.cpp map/map_test_ctr_uuid_long_dflt.cpp)

add_executable(tests ${SRCS})
        
set_target_properties(tests PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_TESTS")
set_target_properties(tests PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        
target_link_libraries(tests Memoria MemoriaTests ${MEMORIA_LIBS})

if (NOT BUILD_MSVC)
    target_link_libraries(tests boost_system boost_filesystem)
endif()

