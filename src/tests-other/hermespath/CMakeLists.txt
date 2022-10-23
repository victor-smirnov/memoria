cmake_minimum_required(VERSION 3.8)

find_package(Catch2 CONFIG REQUIRED)

find_path(FAKEIT_INCLUDE_DIRS "boost/fakeit.hpp")




##
## CONFIGURATION
##
# Unit and compliance test confgiuration
set(HERMESPATH_UNITTEST_TARGET_NAME unit)
set(HERMESPATH_COMPLIANCETEST_TARGET_NAME compliance)
set(HERMESPATH_COPY_COMPLIANCETEST_FILES_TARGET_NAME copy_tests)
set(HERMESPATH_COMPLIANCETEST_DATA_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/jmespath.test/tests")

#if (HERMESPATH_BUILD_TESTS)
    ##
    ## UNIT TEST TARGET
    ##
    # create the unit test target
    add_executable(${HERMESPATH_UNITTEST_TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/unit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/grammar_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/parser_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/expressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/identifiernode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rawstringnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/variantnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/interpreter_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/variantvisitoradaptor_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/subexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/literalnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/indexexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/arrayitemnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/flattenoperatornode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/noderank_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/binaryexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bracketspecifiernode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/insertnodeaction_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sliceexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/listwildcardnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hashwildcardnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/multiselectlistnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/multiselecthashnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/notexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/comparatorexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/orexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/andexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/parenexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/pipeexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/currentnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/filterexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/functionexpressionnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/expressionargumentnode_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/leftchildextractor_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/nodeinsertpolicy_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/nodeinsertcondition_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/leftedgeiterator_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/appendutf8action_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/appendescapesequenceaction_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/encodesurrogatepairaction_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/contextvaluevisitoradaptor_test.cpp)
    # configure the linked libraries
    target_link_libraries(${HERMESPATH_UNITTEST_TARGET_NAME}
        ${HERMESPATH_TARGET_NAME} Catch2::Catch2 Catch2::Catch2WithMain FakeIt)

    target_include_directories(${HERMESPATH_UNITTEST_TARGET_NAME} PRIVATE ${FAKEIT_INCLUDE_DIRS})
    target_include_directories(${HERMESPATH_UNITTEST_TARGET_NAME} PRIVATE ../../core/src/hermes/path)

    # add the necessary compile and link flags for generating test coverage if
    # the coverage info is enabled
    if (${HERMESPATH_COVERAGE_INFO})
        set_target_properties(${HERMESPATH_UNITTEST_TARGET_NAME} PROPERTIES
            COMPILE_FLAGS "-fprofile-arcs  -ftest-coverage"
            LINK_FLAGS "-fprofile-arcs  -ftest-coverage -lgcov")
    endif ()
    # create a ctest test for executing the unit test target
    add_test(NAME ${HERMESPATH_UNITTEST_TARGET_NAME}
        COMMAND ${HERMESPATH_UNITTEST_TARGET_NAME})

    ##
    ## COMPLIANCE TEST TARGET
    ##
    # create the unit test target
    add_executable(${HERMESPATH_COMPLIANCETEST_TARGET_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/compliance.cpp)
    # set the definition which specifies the location of the test data
    target_compile_definitions(${HERMESPATH_COMPLIANCETEST_TARGET_NAME} PRIVATE
        "HERMESPATH_COMPLIANCETEST_DATA_PATH=\"${HERMESPATH_COMPLIANCETEST_DATA_PATH}\"")
    # configure the linked libraries
    target_link_libraries(${HERMESPATH_COMPLIANCETEST_TARGET_NAME}
        ${HERMESPATH_TARGET_NAME} Catch2::Catch2 Catch2::Catch2WithMain FakeIt)

    target_include_directories(${HERMESPATH_COMPLIANCETEST_TARGET_NAME} PRIVATE ${FAKEIT_INCLUDE_DIRS})
    target_include_directories(${HERMESPATH_COMPLIANCETEST_TARGET_NAME} PRIVATE ../../core/src/hermes/path)

    # create a ctest test for executing the unit test target
    add_test(NAME ${HERMESPATH_COMPLIANCETEST_TARGET_NAME}
        COMMAND ${HERMESPATH_COMPLIANCETEST_TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
#endif()
