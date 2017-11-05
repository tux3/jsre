cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
enable_language(ASM_NASM)
project(babel)

add_library(${PROJECT_NAME} STATIC babel.asm)
# A bug in NASM requires a / at the end of include paths, and CMake always strips slashes, so we pass it as an opaque option
target_compile_options(${PROJECT_NAME} PRIVATE "-I${PROJECT_SOURCE_DIR}/")
