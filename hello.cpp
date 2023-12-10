#include <stdio.h>
#include <cstdlib>
#include <string>

// https://github.com/linuxkerneltravel/lmp/blob/develop/eBPF_Supermarket/User_Function_Tracer/src/vmem.c#L28
// Above link could be super useful.

void my_func(char * str) {
    fprintf(stdout, str);
    fflush(stdout);
}

int main(int argc, char** argv)
{
    int a = rand();
    fprintf(stdout, "Random number: %s\n", std::to_string(a).c_str());
    fflush(stdout);
    fprintf(stdout, "before the hello\n");
    fflush(stdout);
    char* str = "Hello world!\n";
    my_func(str);
}