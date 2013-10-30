ADD_GEN_EXECUTABLE(str)
ADD_GEN_TEST(str)
ADD_GEN_TEST_CONFIG(str test_1 current)
ADD_GEN_TEST_CONFIG(str test_1 full)
ADD_TEST(str_cli_1
	${CMAKE_CURRENT_BINARY_DIR}/test_str
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --val4 hello
    --config-print current
    )
SET_TESTS_PROPERTIES(str_cli_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: hello")
