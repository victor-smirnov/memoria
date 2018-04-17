set (SRCS )

set (SRCS ${SRCS} reactor/file_stream_test.cpp)
set (SRCS ${SRCS} reactor/pipe_test.cpp)
set (SRCS ${SRCS} reactor/socket_test.cpp)
set (SRCS ${SRCS} reactor/file_block_test.cpp)
set (SRCS ${SRCS} reactor/file_unbuffered_block_test.cpp)

set (SRCS ${SRCS} packed/bitmap/bitmap_test_suite.cpp)
set (SRCS ${SRCS} packed/allocator/palloc_test_suite.cpp)
set (SRCS ${SRCS} packed/codecs/packed_codecs_test_suite.cpp)
set (SRCS ${SRCS} packed/maxtree/packed_maxtree_test_suite.cpp)
set (SRCS ${SRCS} packed/tree/packed_tree_test_suite.cpp)
set (SRCS ${SRCS} packed/array/packed_array_test_suite.cpp)
set (SRCS ${SRCS} packed/sequence/fse/pseq_test_suite.cpp)
set (SRCS ${SRCS} packed/sequence/rle/rleseq_test_suite.cpp)

#set (SRCS ${SRCS} prototype/sample_suite.cpp)

add_executable(tests2 tests2.cpp ${SRCS})

set_target_properties(tests2 PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(tests2 PROPERTIES LINK_FLAGS    "${MEMORIA_LINK_FLAGS}")
        
target_link_libraries(tests2 Memoria MemoriaTests2)

target_link_libraries(tests2 ${MEMORIA_LIBS})

