#ifndef INCLUDE_PROGRAM_DEBUG_INFO_H
#define INCLUDE_PROGRAM_DEBUG_INFO_H

#include <elf/elf++.hh>
#include <dwarf/dwarf++.hh>

class ProgramDebugInfo {
    public:
        ProgramDebugInfo(const std::string& path, uint64_t codeAddressStart);
        int findFunctionAddr(const std::string& func, uint64_t& addr);

    private:
        std::string d_path;
        elf::elf d_elf;
        dwarf::dwarf d_dwarf;
        uint64_t d_codeAddressStart;
};

#endif