
add_definitions("-Wno-ambiguous-reversed-operator")

if (BUILD_CODEGENERATOR_EXPLICITLY)
    find_package(Clang CONFIG REQUIRED)

    #message("==========================|${MEMORIA_CXX_STANDARD}|===========================")

    #message("####=====================|${CLANG_INSTALL_PREFIX}|===========================")

    file(GLOB CTRGEN_SRCS *.cpp)
    add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${GLOBAL_CORE_SRCS})
    set_property(TARGET MemoriaCodeGen PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})



    target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options )
    target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options pybind11::embed)


    target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
    target_include_directories(MemoriaCodeGen PRIVATE include)
    target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

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

    add_definitions(-pthread)

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
link_libraries(absl::base absl::strings absl::hash absl::meta absl::time)

find_package(fmt CONFIG REQUIRED)
link_libraries(fmt::fmt fmt::fmt-header-only)

find_package(Clang CONFIG REQUIRED)

STRING( REPLACE "^^" ";" MEMORIA_CORE_SRCS "${MEMORIA_CORE_SRCS_STR}" )

file(GLOB CTRGEN_SRCS *.cpp)
add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${MEMORIA_CORE_SRCS})

target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
target_include_directories(MemoriaCodeGen PRIVATE include)
target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options )
target_link_libraries(MemoriaCodeGen PRIVATE pybind11::embed)
target_link_libraries(MemoriaCodeGen PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)

endif()

