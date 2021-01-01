cmake_minimum_required(VERSION 3.1)

set(CMAKE_LANGUAGE_COMPILER usr/bin/gcc)
project(C_flat)
enable_language(C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-Wall -Wextra")

add_executable(Cb

	# src files
	src/main.c
	src/mem.c
	src/err.c
	src/type.c
	src/symbol.c

	src/f_parser.c
	src/f_lexer.c
	src/f_expr.c
	src/f_ast.c
)
