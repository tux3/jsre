## Passes library
function(add_function_passes basename_list)
    foreach(basename ${ARGV})
        list(APPEND FUNCTION_PASSES "X(${basename})")
        string(TOLOWER ${basename} filename)
        set(PASSES_SRCS "${PASSES_SRCS}" "passes/function/${filename}.cpp")
    endforeach(basename)
    set(PASSES_SRCS "${PASSES_SRCS}" PARENT_SCOPE)
    configure_file(${PROJECT_SOURCE_DIR}/passes/function/list.hpp.in "${PROJECT_BINARY_DIR}/generated/passes/function/list.hpp")
endfunction(add_function_passes)

list(APPEND PASSES_SRCS passes/function/list.cpp passes/function/list.hpp)
add_function_passes(missingAwait)

add_library(${PROJECT_NAME}_passes OBJECT "${PASSES_SRCS}")
target_include_directories(${PROJECT_NAME}_passes PRIVATE ${V8_INCLUDE_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/generated)
set_target_properties(${PROJECT_NAME}_passes PROPERTIES CXX_STANDARD 17
                                                 CXX_STANDARD_REQUIRED 1)
