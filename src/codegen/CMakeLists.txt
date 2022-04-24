
add_definitions("-Wno-ambiguous-reversed-operator")

if (BUILD_CODEGENERATOR_EXPLICITLY)
    find_package(Clang CONFIG REQUIRED)
    find_package(pybind11 CONFIG REQUIRED)

    file(GLOB CTRGEN_SRCS *.cpp)
    add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${GLOBAL_CORE_SRCS})
    set_property(TARGET MemoriaCodeGen PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

    target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
    target_include_directories(MemoriaCodeGen PRIVATE include)
    target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

    target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options pybind11::embed pybind11::module util)
    target_link_libraries(MemoriaCodeGen PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)
else()

project(MemoriaCodeGen)
cmake_minimum_required(VERSION 3.19.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Memoria.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(../../include)
include_directories(../core/include)

add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-DMMA_ICU_CXX_NS=icu)

if (BUILD_POSIX)
    add_definitions(-fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors  -ftemplate-backtrace-limit=0)
    add_definitions(-Wno-comment -Wno-unknown-warning-option -Wunused-variable)
    add_definitions(-Wno-unused-function -Wno-unused-but-set-variable)
    add_definitions(-Wno-unused-private-field -Wno-pessimizing-move)
    add_definitions(-Wno-missing-braces)
    add_definitions(-Wno-variables)
    add_definitions(-Werror=unused-result)
    add_definitions(-faligned-new)
    add_definitions(-Wno-class-memaccess)
    add_definitions(-Werror=return-type)

    #add_definitions(-pthread -nostdinc++ -isystem/home/victor/jenny-r15-rel/include/c++/v1  )
    add_definitions(-pthread -stdlib=libstdc++)

    add_global_libs(dl pthread)
else()

endif()

if (MSVC)
    set (Boost_USE_STATIC_RUNTIME    ON)
    set (Boost_USE_STATIC_LIBS       ON)
    find_package(ICU 60.2 COMPONENTS dt i18n uc REQUIRED)
    link_libraries(ICU::dt ICU::uc ICU::i18n)
else()
    set (Boost_USE_STATIC_RUNTIME    ${BUILD_WITH_STATIC_BOOST})
    set (Boost_USE_STATIC_LIBS       ${BUILD_WITH_STATIC_BOOST})
    find_package(ICU 60.2 COMPONENTS i18n uc data REQUIRED)
    link_libraries(ICU::i18n ICU::uc ICU::data)
endif()


find_package(pybind11 CONFIG REQUIRED)

find_package(Boost 1.75.0 COMPONENTS context program_options serialization unit_test_framework REQUIRED)

find_package(absl CONFIG REQUIRED)
link_libraries(absl::base absl::strings absl::hash absl::meta absl::time util)

find_package(fmt CONFIG REQUIRED)
link_libraries(fmt::fmt fmt::fmt-header-only)

find_package(Clang CONFIG REQUIRED)

STRING( REPLACE "^^" ";" MEMORIA_CORE_SRCS "${MEMORIA_CORE_SRCS_STR}" )

#set (MEMORIA_CORE_SRCS "/home/victor/cxx/memoria/src/core/src/exceptions.cpp;/home/victor/cxx/memoria/src/core/src/fixed_size_datum.cpp;/home/victor/cxx/memoria/src/core/src/graph.cpp;/home/victor/cxx/memoria/src/core/src/icu_regexp.cpp;/home/victor/cxx/memoria/src/core/src/ld_document.cpp;/home/victor/cxx/memoria/src/core/src/ld_sdn_qi_parser.cpp;/home/victor/cxx/memoria/src/core/src/ld_type_decl.cpp;/home/victor/cxx/memoria/src/core/src/ldd_array.cpp;/home/victor/cxx/memoria/src/core/src/ldd_map.cpp;/home/victor/cxx/memoria/src/core/src/ldd_typed_value.cpp;/home/victor/cxx/memoria/src/core/src/ldd_value.cpp;/home/victor/cxx/memoria/src/core/src/md5.cpp;/home/victor/cxx/memoria/src/core/src/metadata_tools.cpp;/home/victor/cxx/memoria/src/core/src/models.cpp;/home/victor/cxx/memoria/src/core/src/object_pool.cpp;/home/victor/cxx/memoria/src/core/src/profile_metadata.cpp;/home/victor/cxx/memoria/src/core/src/random.cpp;/home/victor/cxx/memoria/src/core/src/reflection.cpp;/home/victor/cxx/memoria/src/core/src/result.cpp;/home/victor/cxx/memoria/src/core/src/static_core_init.cpp;/home/victor/cxx/memoria/src/core/src/stream.cpp;/home/victor/cxx/memoria/src/core/src/strings.cpp;/home/victor/cxx/memoria/src/core/src/terminal.cpp;/home/victor/cxx/memoria/src/core/src/time.cpp;/home/victor/cxx/memoria/src/core/src/type_registry.cpp;/home/victor/cxx/memoria/src/core/src/ustrings.cpp;/home/victor/cxx/memoria/src/core/src/utext_iterator.cpp;/home/victor/cxx/memoria/src/core/src/utext_strings.cpp;/home/victor/cxx/memoria/src/core/src/uuid.cpp;/home/victor/cxx/memoria/src/core/src/varchars.cpp;/home/victor/cxx/memoria/src/core/src/datatypes/dt_ldd_core.cpp;/home/victor/cxx/memoria/src/core/src/datatypes/dt_simple_numerics.cpp;/home/victor/cxx/memoria/src/core/src/datatypes/dt_system_core.cpp;/home/victor/cxx/memoria/src/core/platforms/posix/posix_exceptions.cpp;/home/victor/cxx/memoria/src/core/platforms/posix/posix_logs.cpp;/home/victor/cxx/memoria/src/core/platforms/posix/posix_platform.cpp;/home/victor/cxx/memoria/src/core/platforms/posix/posix_stream.cpp;/home/victor/cxx/memoria/src/core/platforms/posix/posix_terminal.cpp;/home/victor/cxx/memoria/src/filesystem_common/codecvt_error_category.cpp;/home/victor/cxx/memoria/src/filesystem_common/exception.cpp;/home/victor/cxx/memoria/src/filesystem_common/path.cpp;/home/victor/cxx/memoria/src/filesystem_common/path_traits.cpp;/home/victor/cxx/memoria/src/filesystem_common/portability.cpp;/home/victor/cxx/memoria/src/filesystem_common/utf8_codecvt_facet.cpp;/home/victor/cxx/memoria/src/filesystem_common/windows_file_codecvt.cpp;/home/victor/cxx/memoria/src/filesystem_classic/directory.cpp;/home/victor/cxx/memoria/src/filesystem_classic/operations.cpp;/home/victor/cxx/memoria/src/filesystem_classic/unique_path.cpp")

message("${MEMORIA_CORE_SRCS}")

file(GLOB CTRGEN_SRCS *.cpp)
add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${MEMORIA_CORE_SRCS})

target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
target_include_directories(MemoriaCodeGen PRIVATE include)
target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options )
target_link_libraries(MemoriaCodeGen PRIVATE pybind11::embed pybind11::module)
target_link_libraries(MemoriaCodeGen PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling c++)

target_link_options(MemoriaCodeGen PRIVATE -stdlib=libstdc++)

endif()

