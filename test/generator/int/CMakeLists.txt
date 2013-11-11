ADD_GEN_EXECUTABLE(int)
ADD_GEN_TEST(int)
ADD_GEN_TEST_CONFIG(int test_1 current)
ADD_GEN_TEST_CONFIG(int test_1 full)
ADD_TEST(int_err_1
	${CMAKE_CURRENT_BINARY_DIR}/test_int --val2 not_a_number)
SET_TESTS_PROPERTIES(int_err_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "Option \"--val2\": Integer expected")
ADD_TEST(int_err_2
	${CMAKE_CURRENT_BINARY_DIR}/test_int --val3 100)
SET_TESTS_PROPERTIES(int_err_2 PROPERTIES
	PASS_REGULAR_EXPRESSION "Option \"--val3\": Integer too big, max 10")
ADD_TEST(int_err_3
	${CMAKE_CURRENT_BINARY_DIR}/test_int --val3 0)
SET_TESTS_PROPERTIES(int_err_3 PROPERTIES
	PASS_REGULAR_EXPRESSION "Option \"--val3\": Integer too low, min 5")
ADD_TEST(int_err_4
	${CMAKE_CURRENT_BINARY_DIR}/test_int
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_2.yaml)
SET_TESTS_PROPERTIES(int_err_4 PROPERTIES
	PASS_REGULAR_EXPRESSION "test_2\\.yaml\":1: Integer too big, max 10")
ADD_TEST(int_err_5
	${CMAKE_CURRENT_BINARY_DIR}/test_int
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_3.yaml)
SET_TESTS_PROPERTIES(int_err_5 PROPERTIES
	PASS_REGULAR_EXPRESSION "test_3\\.yaml\":1: Integer too low, min 5")
