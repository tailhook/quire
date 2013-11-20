ADD_GEN_EXECUTABLE(float)
ADD_GEN_TEST(float)
ADD_GEN_TEST_CONFIG(float test_1 current)
ADD_GEN_TEST_CONFIG(float test_1 full)
ADD_TEST(float_err_1
	${CMAKE_CURRENT_BINARY_DIR}/test_float
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --val4 not_a_number)
SET_TESTS_PROPERTIES(float_err_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "Option \"--val4\": Float expected")
ADD_TEST(float_set_1
	${CMAKE_CURRENT_BINARY_DIR}/test_float
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --val4 3.14159
    --config-print=current)
SET_TESTS_PROPERTIES(float_set_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: 3.14159")

