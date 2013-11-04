ADD_GEN_EXECUTABLE(vars)
ADD_GEN_TEST(vars)
ADD_GEN_TEST_CONFIG(vars test_1 current)
ADD_TEST(vars_cli_1
	${CMAKE_CURRENT_BINARY_DIR}/test_vars
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-var a=value
    --config-var ten=0x10
    --config-print current
    )
SET_TESTS_PROPERTIES(vars_cli_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "- 32x value\n")
ADD_TEST(vars_cli_2
	${CMAKE_CURRENT_BINARY_DIR}/test_vars
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-var ten=0x80
    --config-print current
    )
SET_TESTS_PROPERTIES(vars_cli_2 PROPERTIES
	PASS_REGULAR_EXPRESSION "- 49411\n")
