add_subdirectory(samples)

# Create executable for the debugger
add_executable(pgdb main.cpp procMemMap.cpp debugger.cpp programDebugInfo.cpp)
# Add directories to find headers and statically linked code
target_include_directories(pgdb PRIVATE . ${PROJECT_SOURCE_DIR}/include/libelfin)

# Add debugging symbols to the compile command
# This option adds a mapping from the compiled instructions
# to the pre-compiled line/file/symbol
target_compile_options(pgdb PRIVATE -ggdb -include ${PROJECT_SOURCE_DIR}/src/fprintfAddExecutableName.h)

## Add libelfin shared library dependency
# Build libelfin
add_custom_target(
    libelfin
    COMMAND make
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/libelfin
)
# Link in libelfin
target_link_libraries(pgdb
    ${PROJECT_SOURCE_DIR}/include/libelfin/dwarf/libdwarf++.so
    ${PROJECT_SOURCE_DIR}/include/libelfin/elf/libelf++.so)
# Require libelfin as a dependency for pgdb
add_dependencies(pgdb libelfin)