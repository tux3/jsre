cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(bhom)

## Babel transpiler lib
add_subdirectory(babel)

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


## Main project target
function(add_headers_sources basename_list)
    foreach(basename ${ARGV})
        set(SRCS "${SRCS}" "${basename}.cpp" "${basename}.hpp")
    endforeach(basename)
    set(SRCS "${SRCS}" PARENT_SCOPE)
endfunction(add_headers_sources)

list(APPEND SRCS main.cpp)
add_headers_sources(
    v8 isolatewrapper utils babel global reporting
    module/basicmodule module/nativemodule module/module module/moduleresolver module/native/modules
    ast/ast ast/walk ast/import ast/location
    transform/blank transform/flow
    analyze/identresolution analyze/unused analyze/astqueries
)

# TODO: Make a half-hearted attempt at calling a FindV8.cmake or pkg-config
set(V8_LIBS_DIR "/code/build/v8/v8/out.gn/x64.release")
set(V8_INCLUDE_DIR "/code/build/v8/v8/include")

find_library(V8_LIB_V8 NAMES v8 PATHS ${V8_LIBS_DIR})
find_library(V8_LIB_BASE NAMES v8_libbase PATHS ${V8_LIBS_DIR})
find_library(V8_LIB_PLATFORM NAMES v8_libplatform PATHS ${V8_LIBS_DIR})

find_library(ICUUC_LIB NAMES icuuc)
find_library(ICUI18N_LIB NAMES icui18n)

add_executable(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PRIVATE ${V8_INCLUDE_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(${PROJECT_NAME} PRIVATE babel native_modules_js ${V8_LIB_V8} ${V8_LIB_BASE} ${V8_LIB_PLATFORM} ${ICUUC_LIB} ${ICUI18N_LIB} "stdc++fs")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17
                                                 CXX_STANDARD_REQUIRED 1)
