cmake_minimum_required(VERSION 3.10)

project(Debugger)

# Create executable for the debugger
add_executable(pgdb main.cpp)
# Add debugging symbols to the compile command
# This option adds a mapping from the compiled instructions
# to the pre-compiled line/file/symbol
target_compile_options(pgdb PRIVATE -ggdb)

# Create executable for the sample program
add_executable(hello.tsk hello.cpp)
