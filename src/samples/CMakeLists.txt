# Create executable for the sample program
add_executable(hello.tsk hello.cpp)

# Add debugging symbols to the compile command
# This option adds a mapping from the compiled instructions
# to the pre-compiled line/file/symbol
target_compile_options(hello.tsk PRIVATE -gdwarf-2 -include ${PROJECT_SOURCE_DIR}/src/fprintfAddExecutableName.h)