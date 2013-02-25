CMAKE_MINIMUM_REQUIRED(VERSION 2.8)  # feel free to try lower
ENABLE_TESTING()
PROJECT(quire)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu99")


ADD_LIBRARY(quire
    src/yparser.c
    src/metadata.c
    src/access.c
    src/preprocessing.c
    src/genheader.c
    src/gensource.c
    src/options.c
    src/cutil.c
    src/error.c
    src/emitter.c
    src/vars.c
    src/eval.c
    src/access.c
    src/maputil.c
    src/wrappers.c
    objpath/objpath.c
    )

ADD_EXECUTABLE(quire-gen
    src/coyaml.c
    )
TARGET_LINK_LIBRARIES(quire-gen quire)

ADD_EXECUTABLE(quire-tool
    src/ytool.c
    objpath/objpath.c
    )
TARGET_LINK_LIBRARIES(quire-tool quire)

GET_TARGET_PROPERTY(QUIRE_GEN quire-gen LOCATION)

MACRO(ADD_TEST_BINARY name)
    ADD_EXECUTABLE(${name} test/${name}.c ${name}-config.c)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}-config.h ${name}-config.c
        COMMAND ${QUIRE_GEN}
            --source ${CMAKE_CURRENT_SOURCE_DIR}/test/${name}.yaml
            --c-header ${name}-config.h
            --c-source ${name}-config.c
        DEPENDS test/${name}.yaml ${QUIRE_GEN}
        )
    SET_SOURCE_FILES_PROPERTIES(${name}-config.h ${name}-config.c
        PROPERTIES GENERATED 1
        )
    SET_SOURCE_FILES_PROPERTIES(test/${name}.c PROPERTIES
        COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")
    TARGET_LINK_LIBRARIES(${name} quire)
ENDMACRO(ADD_TEST_BINARY)

MACRO(ADD_TOOL_TEST name)
    ADD_TEST(${name} ${CMAKE_SOURCE_DIR}/test_quire_tool
        ${CMAKE_SOURCE_DIR}/test/parser/${name}.test)
ENDMACRO(ADD_TOOL_TEST)

ADD_TEST_BINARY(tinytest)
ADD_TEST(tinytest ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/tinytest
    ${CMAKE_BINARY_DIR}/tinytest.out
    ${CMAKE_SOURCE_DIR}/examples/tinyexample.out
    -c ${CMAKE_SOURCE_DIR}/examples/tinyexample.yaml -vv -C -P)
ADD_TEST_BINARY(vartest)
ADD_TEST(vartest ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/vartest
    ${CMAKE_BINARY_DIR}/vartest.out
    ${CMAKE_SOURCE_DIR}/examples/varexample.out
    -c ${CMAKE_SOURCE_DIR}/examples/varexample.yaml -C -P)
ADD_TEST_BINARY(comprehensive)
ADD_TEST(comprtest ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/comprehensive
    ${CMAKE_BINARY_DIR}/compexample.out
    ${CMAKE_SOURCE_DIR}/examples/compexample.out
    -c ${CMAKE_SOURCE_DIR}/examples/compexample.yaml -C -P
    --config-var clivar=CLI)
ADD_TEST(comprtest_comments ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/comprehensive
    ${CMAKE_BINARY_DIR}/compexample_comments.out
    ${CMAKE_SOURCE_DIR}/examples/compexample_comments.out
    -c ${CMAKE_SOURCE_DIR}/examples/compexample.yaml -C -PP
    --config-var clivar=CLI)
ADD_TEST(comprtest_2 ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/comprehensive
    ${CMAKE_BINARY_DIR}/compr.out
    ${CMAKE_SOURCE_DIR}/examples/compr.out
    -c ${CMAKE_SOURCE_DIR}/examples/compexample.yaml -C -PP
    --config-var clivar=CLI)
ADD_TEST_BINARY(recursive)
ADD_TEST(recursive ${CMAKE_SOURCE_DIR}/test_quire_gen
    ${CMAKE_BINARY_DIR}/recurcive
    ${CMAKE_BINARY_DIR}/recexample.out
    ${CMAKE_SOURCE_DIR}/examples/recexample.out
    -c ${CMAKE_SOURCE_DIR}/examples/recexample.yaml -C -P)

ADD_TOOL_TEST(anchors)
ADD_TOOL_TEST(dictdict)
ADD_TOOL_TEST(dictlist)
ADD_TOOL_TEST(dictnull)
ADD_TOOL_TEST(dict)
ADD_TOOL_TEST(duplicatekey)
ADD_TOOL_TEST(flow)
ADD_TOOL_TEST(list)
ADD_TOOL_TEST(listlist)
ADD_TOOL_TEST(scalar)

INCLUDE_DIRECTORIES(BEFORE SYSTEM include)  # needed for test binaries
