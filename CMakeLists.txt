cmake_minimum_required(VERSION 3.1)

set(CMAKE_LANGUAGE_COMPILER usr/bin/gcc)
project(C_flat)
enable_language(C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-Wall -Wextra")

add_executable(Cb

	# src files
	src/main.c

	src/frontend/sym.c
	src/frontend/ast.c
	src/frontend/def.c
	src/frontend/lex.c
	src/frontend/type.c

	src/backend/ssa.c
)
