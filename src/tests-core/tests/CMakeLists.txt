set (SRCS )

if(BUILD_TESTS_REACTOR)
    set (SRCS ${SRCS} reactor/file_stream_test.cpp)
    set (SRCS ${SRCS} reactor/pipe_test.cpp)
    set (SRCS ${SRCS} reactor/socket_test.cpp)
    set (SRCS ${SRCS} reactor/file_block_test.cpp)
    set (SRCS ${SRCS} reactor/file_unbuffered_block_test.cpp)
endif()

if(BUILD_TESTS_SDN)
    set (SRCS ${SRCS} linked/ld_document_tests.cpp)
    set (SRCS ${SRCS} linked/ld_document_compaction_tests.cpp)
    set (SRCS ${SRCS} linked/ldd_array_tests.cpp)
    set (SRCS ${SRCS} linked/ldd_map_tests.cpp)
endif()

if(BUILD_TESTS_PACKED)
    set (SRCS ${SRCS} packed/bitmap/bitmap_test_suite.cpp)
    set (SRCS ${SRCS} packed/allocator/palloc_test_suite.cpp)
    set (SRCS ${SRCS} packed/codecs/packed_codecs_test_suite.cpp)
    set (SRCS ${SRCS} packed/tree/packed_tree_test_suite.cpp)
    set (SRCS ${SRCS} packed/sequence/fse/pseq_test_suite.cpp)
    set (SRCS ${SRCS} packed/sequence/rle/rleseq_test_suite.cpp)
endif()

if(BUILD_TESTS_CONTAINERS AND BUILD_COW_PROFILE)
set (SRCS ${SRCS} prototype/btss/btss_test_suite.cpp)
set (SRCS ${SRCS} set/set_test_suite.cpp)
set (SRCS ${SRCS} multimap/multimap_test_suite.cpp)
endif()

if(BUILD_TESTS_DATATYPES)
    set (SRCS ${SRCS} integer/integer_test_suite.cpp)
endif()



add_executable(tests2 tests2.cpp ${SRCS})

set_target_properties(tests2 PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(tests2 PROPERTIES LINK_FLAGS    "${MEMORIA_LINK_FLAGS}")

target_link_libraries(tests2 MemoriaStoresReactor MemoriaCtrsReactor MemoriaCoreReactor MemoriaInitReactor MemoriaTests2 yaml-cpp)

target_link_libraries(tests2 ${MEMORIA_LIBS})

