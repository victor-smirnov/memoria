set (SRCS )

add_executable(tests-rr)

if(BUILD_TESTS_REACTOR)
    set (SRCS ${SRCS} reactor/file_stream_test.cpp)
    set (SRCS ${SRCS} reactor/pipe_test.cpp)
    set (SRCS ${SRCS} reactor/socket_test.cpp)
    set (SRCS ${SRCS} reactor/file_block_test.cpp)
    set (SRCS ${SRCS} reactor/file_unbuffered_block_test.cpp)
endif()

if(BUILD_TESTS_SDN OR BUILD_TESTS_HERMES)
    set (SRCS ${SRCS} hermes/document_tests.cpp)
    set (SRCS ${SRCS} hermes/document_compaction_tests.cpp)
    set (SRCS ${SRCS} hermes/array_tests.cpp)
    set (SRCS ${SRCS} hermes/map_tests.cpp)
endif()

if(BUILD_TESTS_PACKED)
    #set (SRCS ${SRCS} packed/bitmap/bitmap_test_suite.cpp)
    #set (SRCS ${SRCS} packed/allocator/palloc_test_suite.cpp)
    #set (SRCS ${SRCS} packed/codecs/packed_codecs_test_suite.cpp)
    set (SRCS ${SRCS} packed/tree/packed_tree_test_suite.cpp)
    #set (SRCS ${SRCS} packed/sequence/fse/pseq_test_suite.cpp)
    #set (SRCS ${SRCS} packed/sequence/ssrle/ssrleseq_test_suite.cpp)

    file (GLOB_RECURSE PSEQ_SRCS packed/sequence/ssrle/*.cpp packed/sequence/ssrle/*.hpp)
    target_sources(tests-rr PRIVATE ${PSEQ_SRCS})
endif()

if(BUILD_TESTS_CONTAINERS AND BUILD_COW_PROFILE)
  set (SRCS ${SRCS} set/set_test_suite.cpp)
  set (SRCS ${SRCS} map/map_test_suite.cpp)
  set (SRCS ${SRCS} multimap/multimap_test_suite.cpp)
  set (SRCS ${SRCS} sequence/sequence_test_suite.cpp)
endif()

if(BUILD_TESTS_DATATYPES)
    set (SRCS ${SRCS} integer/integer_test_suite.cpp)
endif()

if(BUILD_TESTS_STORES)
    set (SRCS ${SRCS} store/swmr_store_test_suite.cpp)

    file (GLOB_RECURSE TEST_SRCS store/*.cpp store/*.hpp)
    target_sources(tests-rr PRIVATE ${TEST_SRCS})
endif()


set_target_properties(tests-rr PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
target_link_libraries(tests-rr PUBLIC Stores Containers AppInit ReactorTests2 )
target_sources(tests-rr PRIVATE tests2.cpp ${SRCS})

#install(TARGETS
#  tests-rr
#  EXPORT MemoriaTargets FILE_SET PROGRAMS
#)
