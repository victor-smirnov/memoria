set (SRCS )

#set (SRCS ${SRCS} reactor/file_stream_test.cpp)
#set (SRCS ${SRCS} reactor/pipe_test.cpp)
#set (SRCS ${SRCS} reactor/socket_test.cpp)
#set (SRCS ${SRCS} reactor/file_block_test.cpp)
#set (SRCS ${SRCS} reactor/file_unbuffered_block_test.cpp)

#set (SRCS ${SRCS} packed/bitmap/bitmap_test_suite.cpp)
#set (SRCS ${SRCS} packed/allocator/palloc_test_suite.cpp)
#set (SRCS ${SRCS} packed/codecs/packed_codecs_test_suite.cpp)
#set (SRCS ${SRCS} packed/maxtree/packed_maxtree_test_suite.cpp)
#set (SRCS ${SRCS} packed/tree/packed_tree_test_suite.cpp)
set (SRCS ${SRCS} packed/array/packed_array_test_suite.cpp)
#set (SRCS ${SRCS} packed/sequence/fse/pseq_test_suite.cpp)
#set (SRCS ${SRCS} packed/sequence/rle/rleseq_test_suite.cpp)

#set (SRCS ${SRCS} prototype/btss/btss_test_suite.cpp prototype/btss/btss_test_ctr_dflt.cpp)
#set (SRCS ${SRCS} prototype/btfl/btfl_test_suite.cpp prototype/btfl/btfl_test_ctr_dflt.cpp)

#set (SRCS ${SRCS} map/map_test_suite.cpp map/map_test_ctr_uuid_long_dflt.cpp)
#set (SRCS ${SRCS} vector/vector_test_suite.cpp vector/vector_test_ctr_int_dflt.cpp vector/vector_test_ctr_vint_dflt.cpp)
#set (SRCS ${SRCS} multimap/multimap_test_suite.cpp multimap/multimap_dbl_i64_dflt.cpp multimap/multimap_i64_i64_dflt.cpp multimap/multimap_string_i64_dflt.cpp)
#set (SRCS ${SRCS} sequence/sequence_test_suite.cpp sequence/sequence_ctr_dflt.cpp)

#set (SRCS ${SRCS} integer/integer_test_suite.cpp)

add_executable(tests2 tests2.cpp ${SRCS})

set_target_properties(tests2 PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(tests2 PROPERTIES LINK_FLAGS    "${MEMORIA_LINK_FLAGS}")
        
target_link_libraries(tests2 Memoria MemoriaTests2)

target_link_libraries(tests2 ${MEMORIA_LIBS})

