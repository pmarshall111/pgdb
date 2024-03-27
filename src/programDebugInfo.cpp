#include <programDebugInfo.h>

#include <fcntl.h>

ProgramDebugInfo::ProgramDebugInfo(const std::string &path, uint64_t codeAddressStart) : d_path(path), d_codeAddressStart(codeAddressStart) {
    int fd = open(path.c_str(), O_RDONLY);
    d_elf = elf::elf{elf::create_mmap_loader(fd)};
    d_dwarf = dwarf::dwarf{dwarf::elf::create_loader(d_elf)};
};

int ProgramDebugInfo::findFunctionAddr(const std::string &func, uint64_t &addr) {
    for (auto &cu : d_dwarf.compilation_units()) {
        for (const auto &die : cu.root()) {
            if (die.tag == dwarf::DW_TAG::subprogram && die.has(dwarf::DW_AT::name) && die.has(dwarf::DW_AT::low_pc)) {
                fprintf(stdout, "Found a function called: %s\n", die[dwarf::DW_AT::name].as_string().c_str());
                fprintf(stdout, "Func addr: %d\n", die[dwarf::DW_AT::low_pc].as_address());
                if (func == die[dwarf::DW_AT::name].as_string()) {
                    addr = die[dwarf::DW_AT::low_pc].as_address() + d_codeAddressStart;
                    return 0;
                }
            }
        }
    }
    return 1;
}
