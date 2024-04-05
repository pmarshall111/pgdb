# Create executable for the sample program
add_executable(int3.tsk int3.cpp)

# Add debugging symbols to the compile command
# This option adds a mapping from the compiled instructions
# to the pre-compiled line/file/symbol
target_compile_options(int3.tsk PRIVATE -ggdb)