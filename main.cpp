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

#include <elf/elf++.hh>
#include <dwarf/dwarf++.hh>
#include <fcntl.h>

#include <fstream>

#include <iostream>

// https://github.com/TartanLlama/minidbg/blob/tut_break/include/breakpoint.hpp
// ^ This seems to be a great tutorial on how to do it.

void run_program(const char* programname);
void debugChild(pid_t child_pid, const std::string& memStartHex, uint64_t break_address);

AssignedMemory getStartMemoryAddress(const std::string& filePath, pid_t pid) {
    // Due to Adress Space Layout Randomisation, the start address will be different for every run.
    ProcMemMap pmm;
    pmm.init(pid);
    std::vector<AssignedMemory> mem = pmm.getMapsForFilePath(filePath);
    // Should instead seach for the offset 0 of the maps.
    auto memContainingCode = std::find_if(mem.begin(), mem.end(), [](AssignedMemory am) {return am.offset() == "00000000";});
    if (memContainingCode == mem.end()) {
        throw new std::runtime_error("Could not find memory map for executable code");
    }
    return *memContainingCode;
}

#include <inttypes.h>

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: pgdb <program> <func_to_set_breakpoint_on>\n");
        return -1;
    }
    char * programName = argv[1];
    char * funcName = argv[2];

    int fd = open(programName, O_RDONLY);
    auto elfMaps = elf::elf{elf::create_mmap_loader(fd)};
    auto dwarfMaps = dwarf::dwarf{dwarf::elf::create_loader(elfMaps)};
    uint64_t funcAddr;
    bool foundFunc = false;
    for (auto &cu : dwarfMaps.compilation_units()) {
        for (const auto& die : cu.root()) {
            if (die.tag == dwarf::DW_TAG::subprogram && die.has(dwarf::DW_AT::name) && die.has(dwarf::DW_AT::low_pc)) {
                fprintf(stdout, "Found a function called: %s\n", die[dwarf::DW_AT::name].as_string().c_str());
                fprintf(stdout, "Func addr: %d\n", die[dwarf::DW_AT::low_pc].as_address());
                if (0 == strcmp(funcName, die[dwarf::DW_AT::name].as_string().c_str())) {
                    funcAddr = die[dwarf::DW_AT::low_pc].as_address();
                    foundFunc = true;
                }
            }
        }
    }

    if (!foundFunc) {
        fprintf(stderr, "Could not find function '%s'. Please break on an existing function.\n", funcName);
        return -1;
    }

    // Create new process to run the program we'll debug
    pid_t child_pid = fork();
    if (child_pid == 0) {
        run_program(programName);
    }
    else if (child_pid > 0) {
        fprintf(stdout, "debugger started. Child pid: %i\n", child_pid);
        waitpid(child_pid, NULL, 0); // Wait for child to stop on first instruction
        AssignedMemory codeMemory = getStartMemoryAddress("/home/peter/Documents/personalProjects/debugger/build/hello.tsk", child_pid);
        debugChild(child_pid, codeMemory.addressStart(), funcAddr);
    }
    else {
        perror("Fork failed");
        return -1;
    }

    return 0;
}

void run_program(const char* programname)
{
    fprintf(stdout, "target started. will run '%s'\n", programname);

    /* Allow tracing of this process */
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        perror("ptrace");
        return;
    }

    /* Execute program */
    execl(programname, programname, NULL);
}

void printInHex(const std::string& prefix, size_t addr) {
    std::stringstream addrHex;
    addrHex << std::hex << addr;
    fprintf(stdout, "%s: %s.\n", prefix.c_str(), addrHex.str().c_str());
}

void debugChild(pid_t child_pid, const std::string& memStartHex, uint64_t breakAddress)
{
    size_t stop_addr = stol(memStartHex,0,16) + breakAddress;
    printInHex("Breakpoint address", stop_addr);
    uint8_t int3 = 0xCC;
    uint64_t dataToRestore = ptrace(PTRACE_PEEKDATA, child_pid, stop_addr);
    // Change the register to be the int 3 instruction. This will cause the program to stop
    // on the address.
    ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (dataToRestore & ~0xFF) | int3);
    // Tell child to run. It should now run until it hits the breakpoint (int3 instruction).
    ptrace(PTRACE_CONT, child_pid, 0, 0);
    
    /* Wait for child to stop */
    int wait_status;
    waitpid(child_pid, &wait_status, 0);

    while (WIFSTOPPED(wait_status)) { // wait_status is normally 1407, in hex: 0x57F
        fprintf(stdout, "Stopped\n");
        sleep(1);

        // Get the address of the next instruction. This will be the instruction after the memory address we set to
        // int3 since instruction point register (RIP) is incremented after the CPU runs any instruction.
        // We use 8*REG_RIP because this is a 64 bit system, so each register is of length 8 bytes. 
        uint64_t addr = ptrace(PTRACE_PEEKUSER, child_pid, 8 * REG_RIP, NULL); // PEEKUSER is for getting the register values. PEEKDATA is for reading the program data.
        printInHex("Stop address", addr);

        // Replace the data at the stop address with what it should have been
        uint64_t currData = ptrace(PTRACE_PEEKDATA, child_pid, stop_addr);
        fprintf(stdout, "The data: %ld\n", currData);
        fprintf(stdout, "The data to restore: %ld\n", dataToRestore);
        ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (currData & ~0xFF)|dataToRestore);

        // Move RIP register back 1 step now we've replaced the data with what was there before and run it
        ptrace(PTRACE_POKEUSER, child_pid, 8 * REG_RIP, addr-1);
        ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0);

        // Wait for child to stop on its next instruction
        wait(&wait_status);
        printInHex("Address after singlestep", ptrace(PTRACE_PEEKUSER, child_pid, 8 * REG_RIP, NULL));

        // Set the breakpoint again so the next time the code hits the memory address the breakpoint is preserved.
        ptrace(PTRACE_POKEDATA, child_pid, stop_addr, (dataToRestore & ~0xFF) | int3);

        // Allow the program to continue until it next gets interrupted
        ptrace(PTRACE_CONT, child_pid, 0, 0);

        /* Wait for child to stop on its next instruction */
        wait(&wait_status);
    }

    fprintf(stdout, "Completed!\n");
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
