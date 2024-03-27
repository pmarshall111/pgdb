#include <algorithm>
#include <filesystem>

#include <sys/ptrace.h>
#include <sys/wait.h>

#include <debugger.h>
#include <programDebugInfo.h>
#include <procMemMap.h>


void runChild(const char* programname);
AssignedMemory getCodeMemoryAddress(const std::string& filePath, pid_t pid);


int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: pgdb <program>\n");
        return -1;
    }

    // Convert argument to absolute path
    std::filesystem::path path = argv[1];
    auto absPath = std::filesystem::absolute(path);
    const std::string absolutePathProgram = std::filesystem::absolute(path).string();

    // Create new process to run the program we'll debug
    pid_t child_pid = fork();
    if (child_pid == 0) {
        // If we're now in new process, run child program
        runChild(absolutePathProgram.c_str());
    }
    else if (child_pid > 0) {
        // If we're still in parent, start debugger
        fprintf(stdout, "debugger started. Child pid: %i\n", child_pid);
        waitpid(child_pid, NULL, 0); // Wait for child to stop on first instruction
        AssignedMemory codeMemory = getCodeMemoryAddress(absolutePathProgram, child_pid);
        ProgramDebugInfo programDebugInfo(absolutePathProgram, strtol(codeMemory.addressStart().c_str(),0,16));
        Debugger debugger(child_pid, programDebugInfo);
        debugger.start();
    }
    else {
        perror("Fork failed");
        return -1;
    }

    return 0;
}


AssignedMemory getCodeMemoryAddress(const std::string& filePath, pid_t pid) {
    // Due to Adress Space Layout Randomisation, the code address will be different for every run.
    ProcMemMap pmm;
    pmm.init(pid);
    std::vector<AssignedMemory> mem = pmm.getMapsForFilePath(filePath);
    auto memContainingCode = std::find_if(mem.begin(), mem.end(), [](AssignedMemory am) {return am.offset() == "00000000";});
    if (memContainingCode == mem.end()) {
        throw new std::runtime_error("Could not find memory map for executable code");
    }
    return *memContainingCode;
}

void runChild(const char* programname)
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