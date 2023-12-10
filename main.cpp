#include <unistd.h>
#include <algorithm>
#include <stdio.h>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

#include <procMemMap.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

// https://github.com/TartanLlama/minidbg/blob/tut_break/include/breakpoint.hpp
// ^ This seems to be a great tutorial on how to do it.

void run_program(const char* programname);
void count_instructions_in(pid_t child_pid);
void printWithPid(const char* format, ...);

int main(int argc, char** argv)
{
    // if (argc < 2) {
    //     fprintf(stderr, "Expected a program name as argument\n");
    //     return -1;
    // }
    char * programName = "build/hello.tsk";

    // Start process to debug in new thread
    pid_t child_pid = fork();
    if (child_pid == 0)
        run_program(programName);
    else if (child_pid > 0)
        count_instructions_in(child_pid);
    else {
        perror("Fork failed");
        return -1;
    }

    return 0;
}

void run_program(const char* programname)
{
    printWithPid("target started. will run '%s'\n", programname);

    /* Allow tracing of this process */
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        perror("ptrace");
        return;
    }

    /* Execute program */
    execl(programname, programname, NULL);
}

void count_instructions_in(pid_t child_pid)
{
    /* Wait for child to stop on first instruction */
    int wait_status;
    pid_t rc = waitpid(child_pid, &wait_status, 0);
    if (rc != child_pid) {
        printWithPid("error: %d\n", rc);
    }
    printWithPid("Beign waitstatus: %d\n", wait_status);
    printWithPid("debugger started. Child pid: %i\n", child_pid);

    // Get memory assigned to process. This can change on every execution.
    ProcMemMap pmm;
    pmm.init(child_pid);
    std::vector<AssignedMemory> mem = pmm.getMapsForFilePath("/home/peter/Documents/personalProjects/debugger/build/hello.tsk");
    // Should instead seach for the offset 0 of the maps.
    auto memContainingCode = std::find_if(mem.begin(), mem.end(), [](AssignedMemory am) {return am.isExecutable();});
    if (memContainingCode == mem.end()) {
        throw new std::runtime_error("Could not find memory map for executable code");
    }
    AssignedMemory codeMemory = *memContainingCode;

    // Set breakpoint just after we print the random number. 0x13d1 corresponds to line 18 in hello.cpp
    // I've changed this to 0x03d1 because the address should begin from the start of the mem segment.
    size_t stop_addr = stol(codeMemory.addressStart(),0,16) + stol(std::string("00000000000003d1"), 0, 16);
    std::stringstream stopAddrHex;
    stopAddrHex << std::hex << stop_addr << std::endl;
    printWithPid("Address start in hex: %s. Breakpoint in hex: %s\n", codeMemory.addressStart().c_str(), stopAddrHex.str().c_str());
    uint64_t dataToRestore = ptrace(PTRACE_PEEKDATA, child_pid, stop_addr);
    uint8_t int3 = 0xCC;
    // Change the last byte of the register to be the int 3 instruction. This will cause the program to stop
    // on the address.
    if (ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (dataToRestore & ~0xFF) | int3) < 0) {
        perror("int3");
    };

    // Tell child to run. It should run until it hits the breakpoint.
    if (ptrace(PTRACE_CONT, child_pid, 0, 0) < 0) {
        perror("cont");
    };

    /* Wait for child to stop */
    pid_t out = waitpid(child_pid, &wait_status, 0);
    if (out != child_pid) {
        printWithPid("error: %d\n", out);
    }
    printWithPid("waitstatus: %d\n", wait_status);

    unsigned icounter = 0;
    while (WIFSTOPPED(wait_status)) { // wait_status is normally 1407, in hex: 0x57F
        icounter++;
        printWithPid("Stopped\n");

        // Get the address of the next instruction. This will be the instruction after
        // int3 since RIP register is incremented after the CPU runs any instruction.
        uint64_t addr = ptrace(PTRACE_PEEKUSER, child_pid, 8 * REG_RIP, NULL); // PEEKUSER is for getting the register values. PEEKDATA is for reading the program data.
        
        std::stringstream hexAddr;
        hexAddr << std::hex << addr << std::endl;
        printWithPid("The hex addr: %s\n", hexAddr.str().c_str());

        // Replace the data at the stop address with what it should have been
        uint64_t currData = ptrace(PTRACE_PEEKDATA, child_pid, stop_addr);
        printWithPid("The data: %ld\n", currData);
        printWithPid("The data to restore: %ld\n", dataToRestore);
        if (ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (currData & ~0xFF)|dataToRestore) < 0) {
            perror("replace original data");
        };

        // Move RIP register back 1 step now we've replaced the data
        ptrace(PTRACE_POKEUSER, child_pid, 8 * REG_RIP, addr-1);

        // Make the child execute the step we replaced with the breakpoint
        ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0);

        /* Wait for child to stop on its next instruction */
        wait(&wait_status);

        // print address after singlestep        
        std::stringstream hexAddr2;
        hexAddr2 << std::hex << ptrace(PTRACE_PEEKUSER, child_pid, 8 * REG_RIP, NULL) << std::endl;
        printWithPid("After singlestep, the hex addr: %s\n", hexAddr2.str().c_str());

        // Set the breakpoint again
        if (ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (dataToRestore & ~0xFF) | int3) < 0) {
            perror("int3 second time");
        };

        // Allow the program to continue
        ptrace(PTRACE_CONT, child_pid, 0, 0);

        /* Wait for child to stop on its next instruction */
        wait(&wait_status);
    }

    printWithPid("the child executed %u instructions\n", icounter);
}

void printWithPid(const char* format, ...)
{
    // va_list is used to access the variable number of arguments.
    // This is denoted by the ellipsis '...'
    va_list ap;
    fprintf(stdout, "[%d] ", getpid());
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
}



/*
 * Current progress is struggling to match the function I want to put a breakpoint on to a
 * point in the code. There's another project that seems to look at the process map, and sets a breakpoint
 * just on that line, but that address doesn't seem to be called in my code.
 * 
 * Maybe it's possible that the program has done its job but just can't exit because all the instructions have a breakpoint added?
 * Not sure this is the case.
 * 
 * https://github.com/linuxkerneltravel/lmp/blob/develop/eBPF_Supermarket/User_Function_Tracer/src/utrace.c#L480
 * 
 * I think I might be getting somewhere.
 * Use the /proc/<pid>/maps file to see how each segment in the executable is mapped to memory addresses.
 * The memory with our code is stored in the version of our file that is executable. So to set a breakpoint for main, we need to load up
 * the map in /proc/ and add the offset from nm to it.
 * 
 * That *should* then work...! Relatively big change. Next step should be to integrate with the /proc/<pid>/maps file to get the memory
 * range.
 * 
 * After that we can do manual tests by hardcoding the offset of the main function since this will not change from run to run.
 * 
*/


/*
* New progress = we have a class to read the /proc/pid/maps file and we should be able to test whether we can now set the breakpoint
* Need to figure out when we have to replace the breakpoint command with the original command
*
* Also need to figure out how to edit my cmakelists to include the procMemMap file.
*/



/*
* Current thoughts are that the offset from nm is actually from the beginning of the block for the file. So an offset of 1e29 in nm would need to add
* to the lowest of the the memory segments for the executable. This is correct
*
* I think just in terms of more understanding I could be printing the file the address is from.
* An extension of that could be to somehow convert the address into a symbol. That sounds much harder, but ultimately is probably something I need anyway.
* It would help me understand what's going on a lot more.
*
* I think that the single step is not suitable for me. Because it goes into the underlying libraries. I need to either be able to singlestep until we get
* back into our application, or find out the address of the next line of code and place a new breakpoint. I think easiest is probably looking at what code
* the library is in. This is the whole step over/step into thing.
*
* Next: Write some code to get the library the address is from.
*       Figure out how to convert an address to a line in code.

* It would be more efficient to know what line we're currently on and then get the address of the next line and keep stepping over till we hit that address.

https://github.com/cwgreene/line2addr/blob/master/line2addr.py
*/
