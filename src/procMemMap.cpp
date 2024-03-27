#include <procMemMap.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unistd.h> 


void ProcMemMap::init(pid_t pid) {
    // Read and parse the /proc/<pid>/maps file.
    std::ifstream file("/proc/" + std::to_string(pid) + "/maps");
    if (file.is_open()) {
        for (std::string line; getline(file, line); ) {
            d_assignedMemory.push_back(AssignedMemory(line));
        }
    }
}

std::vector<AssignedMemory> ProcMemMap::getMapsForFilePath(std::string filePath) {
    std::vector<AssignedMemory> outVec;
    std::copy_if(d_assignedMemory.begin(), 
                d_assignedMemory.end(), 
                std::back_inserter(outVec),
                [filePath](AssignedMemory am) {return am.filePath() == filePath;} );
    return outVec;
}

AssignedMemory::AssignedMemory(const std::string& procMapsLine) {
  std::stringstream ss(procMapsLine);
  std::string token;
  for (int pos = 0; ss >> token; pos++) {
      switch (pos) {
          case 0: {
            int dashPos = token.find("-");
            this->d_addressStart = token.substr(0, dashPos);
            this->d_addressEnd = token.substr(dashPos+1);
            break;
          }
          case 1:
            this->d_perms = token;
            break;
          case 2:
            this->d_offset = token;
            break;
          case 5:
            this->d_filePath = token;
            break;            
      }
  }
}

const std::string& AssignedMemory::filePath() {
    return d_filePath;
}

const std::string& AssignedMemory::addressStart() {
    return d_addressStart;
}

const std::string& AssignedMemory::addressEnd() {
    return d_addressEnd;
}

bool AssignedMemory::isReadable() {
    return d_perms.at(0) == 'r';
}

bool AssignedMemory::isWritable() {
    return d_perms.at(1) == 'w';
}

bool AssignedMemory::isExecutable() {
    return d_perms.at(2) == 'x';
}

const std::string& AssignedMemory::offset() {
    return d_offset;
}

// int main() {
//     ProcMemMap pmm;
//     pmm.init(getpid());
// }