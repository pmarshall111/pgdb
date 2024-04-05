#include <iostream>

/*
 * On X86-64 the int3 assembly instruction will set a breakpoint.
 * You can prove this by running this program in gdb and the program will
 * hit a breakpoint despite not setting one via gdb.
 */

int main() {
    std::cout << "Before breakpoint" << std::endl;
    asm("int $3");
    std::cout << "After breakpoint" << std::endl;
}