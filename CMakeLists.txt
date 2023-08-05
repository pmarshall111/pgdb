cmake_minimum_required(VERSION 3.10)

project(Debugger)

# Create executable for the debugger
add_executable(pgdb main.cpp)

# Create executable for the sample program
add_executable(hello.tsk hello.cpp)
