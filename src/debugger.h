#ifndef INCLUDE_DEBUGGER_H
#define INCLUDE_DEBUGGER_H

#include <optional>

#include <programDebugInfo.h>

class Breakpoint {
  public:
    Breakpoint(uint64_t child_pid, uint64_t stop_addr, const std::string &name);
    void set();
    void unset();
    const std::string &getName();

  private:
    uint64_t d_childPid;
    uint64_t d_stopAddr;
    std::string d_name;
    uint64_t d_dataToRestore;
};

class BreakpointManager {
  public:
    BreakpointManager(ProgramDebugInfo programDebugInfo);
    std::optional<Breakpoint> getByAddress(uint64_t addr);
    void createBreakpoint(std::string funcName, uint64_t childPid);

  private:
    std::map<uint64_t, Breakpoint> d_breakpointStore;
    ProgramDebugInfo d_programDebugInfo;
};

class Debugger {
  public:
    Debugger(uint64_t childPid, ProgramDebugInfo programDebugInfo);
    void start();

  private:
    std::vector<std::string> getUserInput();

    bool isStoppedOnBreakpoint(int waitStatus);
    bool isComplete(int waitStatus);
    void runOriginalInstruction();

    uint64_t d_childPid;
    bool d_programStarted;
    BreakpointManager d_breakpointManager;
};

#endif