set(TEST_SRCS "test/test_main.cpp")

# Generate ident resolution test cases
file(GLOB IDENT_RESOLUTION_TEST_CASE_FILES "${PROJECT_SOURCE_DIR}/test/identresolution/*.js")
string(REPLACE ";" "\", \n\"" IDENT_RESOLUTION_TEST_CASE_FILES "${IDENT_RESOLUTION_TEST_CASE_FILES}")
configure_file("${PROJECT_SOURCE_DIR}/test/identresolution/test.cpp.in" "${PROJECT_BINARY_DIR}/generated/test/identresolution/test.cpp")
list(APPEND TEST_SRCS "${PROJECT_BINARY_DIR}/generated/test/identresolution/test.cpp")

# Main test target
add_executable("${PROJECT_NAME}_tests" ${TEST_SRCS})
target_include_directories("${PROJECT_NAME}_tests" PRIVATE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/test" "${PROJECT_SOURCE_DIR}/test/Catch2")
target_link_libraries("${PROJECT_NAME}_tests" "${PROJECT_NAME}_lib" stdc++fs)
set_target_properties("${PROJECT_NAME}_tests" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED 1)

include(CTest)
include(test/Catch2/Catch.cmake)
catch_discover_tests("${PROJECT_NAME}_tests")
