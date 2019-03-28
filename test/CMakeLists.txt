set(TEST_SRCS "test/test_main.cpp" "test/test.hpp")

function(add_tests_with_sample_files test_dirs)
    foreach(test_dir ${ARGV})
        file(GLOB TEST_CASE_FILES "${PROJECT_SOURCE_DIR}/test/${test_dir}/*.js")
        string(REPLACE ";" "\", \n\"" TEST_CASE_FILES "${TEST_CASE_FILES}")
        configure_file("${PROJECT_SOURCE_DIR}/test/${test_dir}/test.cpp.in" "${PROJECT_BINARY_DIR}/generated/test/${test_dir}/test.cpp")
        list(APPEND TEST_SRCS "${PROJECT_BINARY_DIR}/generated/test/${test_dir}/test.cpp")
    endforeach(test_dir)
    set(TEST_SRCS "${TEST_SRCS}" PARENT_SCOPE)
endfunction(add_native_modules)

add_tests_with_sample_files(
    identresolution
    typecheck/scoping
)

# Main test target
add_executable("${PROJECT_NAME}_tests" ${TEST_SRCS})
target_include_directories("${PROJECT_NAME}_tests" PRIVATE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/test" "${PROJECT_SOURCE_DIR}/test/Catch2")
target_link_libraries("${PROJECT_NAME}_tests" "${PROJECT_NAME}_lib" stdc++fs)
set_target_properties("${PROJECT_NAME}_tests" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED 1)

include(CTest)
include(test/Catch2/Catch.cmake)
catch_discover_tests("${PROJECT_NAME}_tests")
