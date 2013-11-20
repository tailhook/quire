ADD_GEN_EXECUTABLE(bool)
ADD_GEN_TEST(bool)
ADD_GEN_TEST_CONFIG(bool test_1 current)
ADD_GEN_TEST_CONFIG(bool test_1 full)
ADD_TEST(bool_err_1
	${CMAKE_CURRENT_BINARY_DIR}/test_bool
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --val4 test)
SET_TESTS_PROPERTIES(bool_err_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "Option \"--val4\": Boolean expected")
ADD_TEST(bool_set_1
	${CMAKE_CURRENT_BINARY_DIR}/test_bool --val4 yes
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_1 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: true")
ADD_TEST(bool_set_2
	${CMAKE_CURRENT_BINARY_DIR}/test_bool --val4 no
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_2 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: false")
ADD_TEST(bool_set_3
	${CMAKE_CURRENT_BINARY_DIR}/test_bool -e
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_3 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: true")
ADD_TEST(bool_set_4
	${CMAKE_CURRENT_BINARY_DIR}/test_bool -d
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_4 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: false")
ADD_TEST(bool_set_5
	${CMAKE_CURRENT_BINARY_DIR}/test_bool --val4=false -e
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_5 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: true")
ADD_TEST(bool_set_6
	${CMAKE_CURRENT_BINARY_DIR}/test_bool --val4=true -d
    --config ${CMAKE_CURRENT_SOURCE_DIR}/test_1.yaml
    --config-print=current)
SET_TESTS_PROPERTIES(bool_set_6 PROPERTIES
	PASS_REGULAR_EXPRESSION "val4: false")
