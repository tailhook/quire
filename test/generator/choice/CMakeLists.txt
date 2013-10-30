ADD_GEN_EXECUTABLE(choice)
ADD_GEN_TEST(choice)
ADD_GEN_TEST_CONFIG(choice test_1 current)
ADD_GEN_TEST_CONFIG(choice test_1 full)
ADD_TEST(choice_err_1
	${CMAKE_CURRENT_BINARY_DIR}/test_choice -c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_2.yaml)
SET_TESTS_PROPERTIES(choice_err_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "Value error at \"[^\"]*/test_2.yaml\":2: Tag is expected")
