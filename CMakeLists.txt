CMAKE_MINIMUM_REQUIRED(VERSION 2.8)  # feel free to try lower
ENABLE_TESTING()
PROJECT(quire)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu99")


ADD_LIBRARY(quire
    src/yaml/parser.c
    src/yaml/access.c
    src/yaml/anchors.c
    src/yaml/node.c
    src/raw/common.c
    src/raw/fromfile.c
    src/raw/include.c
    src/raw/globseq.c
    src/raw/globmap.c
    src/raw/merge.c
    src/cfg/vars.c
    src/cfg/eval.c
    src/cfg/context.c
    src/cfg/api.c
    src/error.c
    src/emitter.c
    )

ADD_EXECUTABLE(quire-gen
    src/gen/util/print.c
    src/gen/util/fwdecl.c
    src/gen/util/name.c
    src/gen/special/special.c
    src/gen/special/types.c
    src/gen/classes/classes.c
    src/gen/classes/alter.c
    src/gen/classes/enum.c
    src/gen/classes/struct.c
    src/gen/classes/scalar.c
    src/gen/cli.c
    src/gen/context.c
    src/gen/struct.c
    src/gen/process.c
    src/gen/main.c
    src/gen/header.c
    src/gen/source.c
    src/gen/metadata.c
    src/gen/options.c
    )
TARGET_LINK_LIBRARIES(quire-gen quire)

ADD_EXECUTABLE(quire-tool
    src/tool/main.c
    objpath/objpath.c
    )
TARGET_LINK_LIBRARIES(quire-tool quire)

GET_TARGET_PROPERTY(QUIRE_GEN quire-gen LOCATION)

FIND_FILE(ASTYLE astyle)

# MACROS FOR TESTS

MACRO(ADD_TOOL_TEST name)
    ADD_TEST(${name} ${CMAKE_SOURCE_DIR}/test_quire_tool
        ${CMAKE_SOURCE_DIR}/test/parser/${name}.test)
ENDMACRO(ADD_TOOL_TEST)

MACRO(ADD_GEN_EXECUTABLE name)
    ADD_EXECUTABLE(test_${name}
        main.c
        config.c)
    ADD_CUSTOM_COMMAND(
        OUTPUT config.h config.c
        COMMAND ${QUIRE_GEN}
            --source ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
            --c-header config.h
            --c-source config.c
        DEPENDS config.yaml quire-gen
        )
    IF(ASTYLE)
        ADD_CUSTOM_COMMAND(
            TARGET test_${name}
            PRE_BUILD
            COMMAND ${ASTYLE} -Y config.c
            DEPENDS config.c
            )
        ADD_CUSTOM_COMMAND(
            TARGET test_${name}
            PRE_BUILD
            COMMAND ${ASTYLE} -Y config.h
            DEPENDS config.h
            )
    ENDIF()

    SET_SOURCE_FILES_PROPERTIES(
        config.h config.c
        PROPERTIES GENERATED 1
        )
    SET_SOURCE_FILES_PROPERTIES(
        main.c
        COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")
    TARGET_LINK_LIBRARIES(test_${name} quire)
ENDMACRO(ADD_GEN_EXECUTABLE)

MACRO(ADD_GEN_TEST name)
    ADD_TEST(${name}_help
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/help.txt
        ${CMAKE_CURRENT_BINARY_DIR}/help.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        --help
        )
ENDMACRO(ADD_GEN_TEST)

MACRO(ADD_GEN_TEST_CONFIG name config)
    ADD_TEST(${name}_${config}_raw
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${config}.raw.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${config}.raw.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        -c ${config}.yaml
        --config-print=current
        )
    ADD_TEST(${name}_${config}_example
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${config}.example.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${config}.example.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        --config-print=example
        )
    ADD_TEST(${name}_${config}_full
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${config}.full.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${config}.full.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        -c ${config}.yaml
        --config-print=full
        )
ENDMACRO(ADD_GEN_TEST_CONFIG)

INCLUDE_DIRECTORIES(BEFORE SYSTEM include)  # needed for test binaries

# TESTS FOR QUIRE-GEN

ADD_SUBDIRECTORY(test/generator/meta)

# TESTS FOR QUIRE-TOOL

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
ADD_TOOL_TEST(mergemap)
ADD_TOOL_TEST(mergeseq)
ADD_TOOL_TEST(incl)
ADD_TOOL_TEST(incmap)
ADD_TOOL_TEST(incseq)

