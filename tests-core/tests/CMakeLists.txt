add_executable(tests4)
add_executable(tests3)

target_sources(tests3 PRIVATE tests3.cpp)
set_target_properties(tests3 PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
target_link_libraries(tests3 PUBLIC AsioLib Boost::program_options Boost::context)
target_link_options(tests3 PRIVATE)

target_sources(tests4 PRIVATE tests4.cpp ${SRCS})
set_target_properties(tests4 PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
target_link_libraries(tests4 PUBLIC Stores Containers AppInit AsioLib Tests2 Boost::program_options Boost::context)
