#include <debugger.h>

#include <iostream>
#include <sstream>

#include <sys/ptrace.h>
#include <sys/wait.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace {
    void printInHex(const std::string& prefix, size_t addr) {
        std::stringstream addrHex;
        addrHex << std::hex << addr;
        fprintf(stdout, "%s: %s.\n", prefix.c_str(), addrHex.str().c_str());
    }
}

//
// Breakpoint
//

Breakpoint::Breakpoint(uint64_t child_pid, uint64_t stop_addr, const std::string& name): d_childPid(child_pid), d_stopAddr(stop_addr), d_name(name) {};

void Breakpoint::set() {
    printInHex("Breakpoint address", d_stopAddr);
    uint8_t int3 = 0xCC;
    d_dataToRestore = ptrace(PTRACE_PEEKDATA, d_childPid, d_stopAddr);
    // Change the register to be the int 3 instruction. This will cause the program to stop
    // on the address.
    ptrace(PTRACE_POKEDATA, d_childPid, d_stopAddr, (d_dataToRestore & ~0xFF) | int3);
}


void Breakpoint::unset() {
    uint64_t currData = ptrace(PTRACE_PEEKDATA, d_childPid, d_stopAddr);
    // Set memory at stop address back to what it was before int3
    ptrace(PTRACE_POKEDATA, d_childPid, d_stopAddr, (currData & ~0xFF)|d_dataToRestore);
}

const std::string& Breakpoint::getName() {return d_name;}


//
// BreakpointManager
//

BreakpointManager::BreakpointManager(ProgramDebugInfo programDebugInfo): d_programDebugInfo(programDebugInfo) {};

std::optional<Breakpoint> BreakpointManager::getByAddress(uint64_t addr) {
    auto iter = d_breakpointStore.find(addr);
    if (iter == d_breakpointStore.end()) {
        return std::optional<Breakpoint>();
    }
    return std::optional(iter->second);
}

void BreakpointManager::createBreakpoint(std::string funcName, uint64_t childPid) {
    uint64_t addr;
    if (0 != d_programDebugInfo.findFunctionAddr(funcName, addr)) {
        fprintf(stderr, "Could not find function '%s'. Please break on an existing function.\n", funcName.c_str());
        return;
    }
    std::optional<Breakpoint> existingBPoint = getByAddress(addr);
    if (existingBPoint) {
        fprintf(stderr, "Breakpoint already set on '%s'. Breakpoint name: '%s'.\n", funcName, existingBPoint->getName().c_str());
    } else {
        Breakpoint breakpoint(childPid, addr, funcName);
        breakpoint.set();
        d_breakpointStore.insert({addr, breakpoint});
        fprintf(stdout, "Breakpoint set on '%s'.\n", funcName.c_str());
    }
}


//
// Debugger
//

Debugger::Debugger(uint64_t childPid, ProgramDebugInfo programDebugInfo): d_childPid(childPid), d_breakpointManager(programDebugInfo) {};

void Debugger::start() {
    int waitStatus;

    while (!isComplete(waitStatus)) {
        if (isStoppedOnBreakpoint(waitStatus)) {
            // We've stopped on a breakpoint! Let's make sure we run the original instruction so the program
            // executes the same as without the debugger.
            runOriginalInstruction();
        }

        std::vector<std::string> tokens = getUserInput();
        std::string cmd = tokens[0];

        if (cmd[0] == 'b') {
            // breakpoint
            if (tokens.size() == 2) {
                d_breakpointManager.createBreakpoint(tokens[1], d_childPid);
            } else {
                fprintf(stdout, "Unrecognised command. Supported commands are 'breakpoint <func>', 'continue', 'run'\n");
            }
        } else if (cmd[0] == 'c' || cmd[0] == 'r') {
            // continue / run
            d_programStarted = true;
            ptrace(PTRACE_CONT, d_childPid, 0, 0);
            waitpid(d_childPid, &waitStatus, 0); // Wait for child to stop at next breakpoint or exit
        } else {
            fprintf(stdout, "Unrecognised command. Supported commands are 'breakpoint <func>', 'continue', 'run'\n");
        }
    }

    fprintf(stdout, "Execution complete!\n");
}

std::vector<std::string> Debugger::getUserInput() {
    fprintf(stdout, "Enter a command for pgdb to run:\n");

    // Get input
    std::string userInput;
    getline(std::cin, userInput);
    
    // Split user input to tokens
    std::vector<std::string> tokens;
    auto pred = boost::is_any_of(" ");
    boost::split(tokens, userInput, pred);
    return tokens;
}

bool Debugger::isStoppedOnBreakpoint(int waitStatus) {
    if (d_programStarted) {
        return WIFSTOPPED(waitStatus); // WIFSTOPPED(ws) is 0 if child is dead
    }
    return false;
}

bool Debugger::isComplete(int waitStatus) {
    if (!d_programStarted) {
        return false;
    }
    return !WIFSTOPPED(waitStatus); // WIFSTOPPED(ws) is 0 if child is dead
}

void Debugger::runOriginalInstruction() {
    // Get the address the Instruction Pointer is pointing to. This is the address of the next instruction to run.
    // We do 8 * REG_RIP since this is 64 bit system. Each register is 8 bytes.
    uint64_t ripAddr = ptrace(PTRACE_PEEKUSER, d_childPid, 8 * REG_RIP, NULL); // PEEKUSER is for getting the register values. PEEKDATA is for reading the program data.
    printInHex("Stop address", ripAddr-1);
    std::optional<Breakpoint> breakpoint = d_breakpointManager.getByAddress(ripAddr-1); // Breakpoint will be at instruction before Instruction Pointer since IP points at the next instruction to run.
    if (breakpoint) {
        fprintf(stdout, "Stopped at breakpoint name '%s'\n", breakpoint->getName().c_str());
        // Restoring data and stepping 1 step.
        breakpoint->unset();
        // Move RIP register back 1 step now we've replaced the data with what was there before and run it
        ptrace(PTRACE_POKEUSER, d_childPid, 8 * REG_RIP, ripAddr-1);
        ptrace(PTRACE_SINGLESTEP, d_childPid, 0, 0);
        waitpid(d_childPid, NULL, 0);
        // Restore the breakpoint so the next time we get here we stop again
        breakpoint->set();
    } else {
        fprintf(stderr, "BUG. We've stopped but couldn't find the breakpoint.\n");
    }
}
