#include "passes/function/list.hpp"

std::vector<FunctionPass> functionPassList = {
    #define X(PASS) \
        PASS##FunctionPass,
    FUNCTION_PASS_X_LIST
    #undef X
};
