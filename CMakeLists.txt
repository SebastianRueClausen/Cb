cmake_minimum_required(VERSION 3.1)

set(CMAKE_LANGUAGE_COMPILER usr/bin/gcc)
project(C_flat)
enable_language(C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-Wall -Wextra")

add_executable(Cb main.c lex.c gen.c)
