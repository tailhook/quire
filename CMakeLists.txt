CMAKE_MINIMUM_REQUIRED(VERSION 2.8)  # feel free to try lower
PROJECT(quire C)
INCLUDE(CMakeParseArguments)
ENABLE_TESTING()

ADD_LIBRARY(quire
    src/util/parse.c
    src/util/wrap.c
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
    src/raw/template.c
    src/raw/vars.c
    src/raw/eval.c
    src/cfg/context.c
    src/cfg/api.c
    src/cfg/optparser.c
    src/error.c
    src/emitter.c
    )

ADD_EXECUTABLE(quire-gen
    src/gen/util/print.c
    src/gen/util/fwdecl.c
    src/gen/util/name.c
    src/gen/special/special.c
    src/gen/special/types.c
    src/gen/special/include.c
    src/gen/classes/classes.c
    src/gen/classes/struct.c
    src/gen/classes/scalar.c
    src/gen/classes/enum.c
    src/gen/classes/choice.c
    src/gen/types/types.c
    src/gen/types/int.c
    src/gen/types/str.c
    src/gen/types/bool.c
    src/gen/types/float.c
    src/gen/types/array.c
    src/gen/types/map.c
    src/gen/types/type.c
    src/gen/cli.c
    src/gen/context.c
    src/gen/struct.c
    src/gen/process.c
    src/gen/main.c
    src/gen/header.c
    src/gen/source.c
    src/gen/metadata.c
    src/gen/options.c
    src/gen/guard.c
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
    ADD_TEST(NAME ${name}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test/parser
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_quire_tool ${name}.test)
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
        --help)
    ADD_TEST(${name}_example
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/example.txt
        ${CMAKE_CURRENT_BINARY_DIR}/example.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        --config-print=example)
ENDMACRO(ADD_GEN_TEST)

MACRO(ADD_GEN_TEST2 name help example)
    ADD_TEST(${name}_help
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${help}.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${help}.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        --help)
    ADD_TEST(${name}_example
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${example}.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${example}.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        --config-print=example)
ENDMACRO(ADD_GEN_TEST2)

MACRO(ADD_GEN_TEST_CONFIG name config mode)
    ADD_TEST(${name}_${config}_${mode}
        ${CMAKE_SOURCE_DIR}/test_quire_gen
        ${CMAKE_CURRENT_SOURCE_DIR}/${config}.${mode}.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${config}.${mode}.out
        ${CMAKE_CURRENT_BINARY_DIR}/test_${name}
        -c ${CMAKE_CURRENT_SOURCE_DIR}/${config}.yaml
        --config-print=${mode}
        )
ENDMACRO(ADD_GEN_TEST_CONFIG)

INCLUDE_DIRECTORIES(BEFORE SYSTEM include)  # needed for test binaries

IF(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})

# TESTS FOR QUIRE-GEN

ADD_SUBDIRECTORY(test/generator/meta)
ADD_SUBDIRECTORY(test/generator/int)
ADD_SUBDIRECTORY(test/generator/str)
ADD_SUBDIRECTORY(test/generator/array)
ADD_SUBDIRECTORY(test/generator/map)
ADD_SUBDIRECTORY(test/generator/struct)
ADD_SUBDIRECTORY(test/generator/tagscalar)
ADD_SUBDIRECTORY(test/generator/enum)
ADD_SUBDIRECTORY(test/generator/frog)
ADD_SUBDIRECTORY(test/generator/choice)
ADD_SUBDIRECTORY(test/generator/bool)
ADD_SUBDIRECTORY(test/generator/float)
ADD_SUBDIRECTORY(test/generator/arar)
ADD_SUBDIRECTORY(test/generator/mapmap)
ADD_SUBDIRECTORY(test/generator/rec)
ADD_SUBDIRECTORY(test/generator/clionly)
ADD_SUBDIRECTORY(test/generator/if)
ADD_SUBDIRECTORY(test/generator/vars)
ADD_SUBDIRECTORY(test/generator/mapar)
ADD_SUBDIRECTORY(test/generator/bitsets)

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
ADD_TOOL_TEST(longstr)
ADD_TOOL_TEST(listblock)
ADD_TOOL_TEST(exkeys)
ADD_TOOL_TEST(vars)
ADD_TOOL_TEST(template)

ENDIF()

