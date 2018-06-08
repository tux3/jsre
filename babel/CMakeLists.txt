cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
enable_language(ASM_NASM)
project(babel)

add_library(${PROJECT_NAME} STATIC babel.asm)
# A bug in NASM requires a / at the end of include paths, and CMake always strips slashes, so we pass it as an opaque option
target_compile_options(${PROJECT_NAME} PRIVATE "-I${PROJECT_SOURCE_DIR}/")

# OBJECT_DEPENDS is necessary to have a dependency on the .js file, since it's passed in behind the scenes via incbin
set_source_files_properties(
    "${PROJECT_SOURCE_DIR}/babel.asm"
    PROPERTIES OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/babel.js"
)

add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/babel.js"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND yarn -s build
    COMMENT "Generating webpack bundle babel.js"
)
