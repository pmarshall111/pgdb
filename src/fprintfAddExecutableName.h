#ifndef INCLUDE_FPRINTF_MACRO
#define INCLUDE_FPRINTF_MACRO

#include <cstdio>
#include <fstream>

#define EXEC_NAME []{std::string name; std::ifstream("/proc/self/comm") >> name; return name;}() // Immediately invoked lambda. Get exec name from proc dir
#define APPEND_EXECUTABLE_TO_FIRST_ARG(a, ...) ("["+EXEC_NAME+"] "+=a).c_str(), ##__VA_ARGS__ // ## Removes the comma before in case there are 0 args in __VA_ARGS__. CPP 20 has __VA_OPT__ as a replacement 
#define fprintf(FILE, ...) fprintf(FILE, APPEND_EXECUTABLE_TO_FIRST_ARG(__VA_ARGS__))

#endif