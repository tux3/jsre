cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(jsre)

## Config
set(V8_INCLUDE_DIR "" CACHE STRING "v8 include directory")
if ("${V8_INCLUDE_DIR}" STREQUAL "")
    message(FATAL_ERROR "V8_INCLUDE_DIR must be set")
endif()
set(V8_STATIC_LIBS_DIR "" CACHE STRING "v8 static libraries directory")
if ("${V8_STATIC_LIBS_DIR}" STREQUAL "")
    message(FATAL_ERROR "V8_STATIC_LIBS_DIR must be set")
endif()
set(V8_SNAPSHOTS_DIR "" CACHE STRING "v8 binary snapshots directory")
if ("${V8_SNAPSHOTS_DIR}" STREQUAL "")
    message(FATAL_ERROR "V8_SNAPSHOTS_DIR must be set")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

## Babel transpiler lib
add_subdirectory(babel)

## Function and module passes lib
include(passes/CMakeLists.txt)

## Native modules
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

function(add_native_modules basename_list)
    # Link in C++ and JS implementations
    foreach(basename ${ARGV})
        set(SRCS "${SRCS}" "module/native/${basename}.cpp")
        list(APPEND MODULES "X(${basename})")

        if(EXISTS ${PROJECT_SOURCE_DIR}/module/native/${basename}.js)
            set(MODULE_NAME ${basename})
            configure_file(${PROJECT_SOURCE_DIR}/module/native/module_js.asm.in "${PROJECT_BINARY_DIR}/generated/module/native/${basename}_js.asm")
            # OBJECT_DEPENDS is necessary to have a dependency on the *.js files, since they are passed in behind the scenes via incbin
            set_source_files_properties(
                "${PROJECT_BINARY_DIR}/generated/module/native/${basename}_js.asm"
                PROPERTIES OBJECT_DEPENDS ${PROJECT_SOURCE_DIR}/module/native/${basename}.js
            )
            list(APPEND NATIVE_MODULE_JS_SRCS "${PROJECT_SOURCE_DIR}/module/native/${basename}.js")
            list(APPEND NATIVE_MODULE_JS_ASM_SRCS "${PROJECT_BINARY_DIR}/generated/module/native/${basename}_js.asm")
        endif()
    endforeach(basename)
    set(SRCS "${SRCS}" PARENT_SCOPE)
    set(NATIVE_MODULE_JS_SRCS "${NATIVE_MODULE_JS_SRCS}" PARENT_SCOPE)
    set(NATIVE_MODULE_JS_ASM_SRCS "${NATIVE_MODULE_JS_ASM_SRCS}" PARENT_SCOPE)

    # Generate modules list header
    string(REPLACE ";" " \\\n    " MODULES "${MODULES}") # Carefully crafted number of spaces!
    configure_file(${PROJECT_SOURCE_DIR}/module/native/modules.hpp.in "${PROJECT_BINARY_DIR}/generated/module/native/modules.hpp")
endfunction(add_native_modules)

set(NATIVE_MODULE_JS_SRCS)
set(NATIVE_MODULE_JS_ASM_SRCS)
add_native_modules(buffer console fs process util crypto tty)

add_library(native_modules_js STATIC ${NATIVE_MODULE_JS_ASM_SRCS} ${NATIVE_MODULE_JS_SRCS})
# A bug in NASM requires a / at the end of include paths, and CMake always strips slashes, so we pass it as an opaque option
target_compile_options(native_modules_js PRIVATE "-I${PROJECT_SOURCE_DIR}/module/native/")

## v8 startup snapshots target
function(add_v8_startup_snapshot basename_list)
    set(SNAPSHOT_BASE_PATH ${V8_SNAPSHOTS_DIR})
    foreach(basename ${ARGV})
        set(SNAPSHOT_NAME ${basename})
        configure_file(${PROJECT_SOURCE_DIR}/v8_startup_snapshots.asm.in "${PROJECT_BINARY_DIR}/generated/snapshots/${basename}.asm")
        # OBJECT_DEPENDS is necessary to have a dependency on the *.bin files, since they are passed in behind the scenes via incbin
        set_source_files_properties("${PROJECT_BINARY_DIR}/generated/snapshots/${basename}.asm" PROPERTIES OBJECT_DEPENDS "${SNAPSHOT_BASE_PATH}/${basename}.bin")
        list(APPEND STARTUP_SNAPSHOTS_ASM_SRCS "${PROJECT_BINARY_DIR}/generated/snapshots/${basename}.asm")
    endforeach(basename)
    set(STARTUP_SNAPSHOTS_ASM_SRCS "${STARTUP_SNAPSHOTS_ASM_SRCS}" PARENT_SCOPE)
endfunction(add_v8_startup_snapshot)
add_v8_startup_snapshot(natives_blob snapshot_blob)

add_library(startup_snapshots STATIC "${STARTUP_SNAPSHOTS_ASM_SRCS}")
# A bug in NASM requires a / at the end of include paths, and CMake always strips slashes, so we pass it as an opaque option
target_compile_options(startup_snapshots PRIVATE "-I${V8_SNAPSHOTS_DIR}/")

## Main project target
function(add_headers_sources basename_list)
    foreach(basename ${ARGV})
        if(EXISTS "${PROJECT_SOURCE_DIR}/${basename}.cpp")
            set(SRCS "${SRCS}" "${basename}.cpp" "${basename}.hpp")
        else()
            set(SRCS "${SRCS}" "${basename}.hpp")
        endif()
    endforeach(basename)
    set(SRCS "${SRCS}" PARENT_SCOPE)
endfunction(add_headers_sources)

list(APPEND SRCS main.cpp)
add_headers_sources(
    v8 isolatewrapper utils global reporting hash
    module/basicmodule module/nativemodule module/module module/moduleresolver module/native/modules
    ast/ast ast/parse ast/import ast/location ast/walk
    graph/graph graph/graphbuilder graph/dot graph/type graph/basicblock
    transform/blank transform/flow
    analyze/identresolution analyze/astqueries analyze/unused analyze/conditionals analyze/typecheck
    queries/maybe queries/dataflow queries/types queries/typeresolution
)

find_library(ICUUC_LIB NAMES icuuc)
find_library(ICUI18N_LIB NAMES icui18n)

set(V8_STATIC_LIBS "v8_base" "v8_external_snapshot" "v8_libplatform" "v8_libbase" "v8_libsampler")

add_executable(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${V8_INCLUDE_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_passes pthread sodium babel native_modules_js startup_snapshots -L${V8_STATIC_LIBS_DIR} ${V8_STATIC_LIBS} ${ICUUC_LIB} ${ICUI18N_LIB} "stdc++fs")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17
                                                 CXX_STANDARD_REQUIRED 1)
