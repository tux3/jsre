#define CATCH_CONFIG_CONSOLE_WIDTH 120
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "v8/isolatewrapper.hpp"

IsolateWrapper& getIsolateWrapper()
{
    static IsolateWrapper isolateWrapper;
    return isolateWrapper;
}
