#ifndef INCLUDE_PROC_MEM_MAP_H
#define INCLUDE_PROC_MEM_MAP_H

#include <string>
#include <vector>

class AssignedMemory {
  public:
    // Class should be constructed from a line in a /proc/<pid>/maps file.
    // Example line:
    // 5633d0923000-5633d0924000 r--p 00000000 103:05 6977766                   /home/peter/Documents/personalProjects/debugger/build/hello.tsk
    // 5633d0924000-5633d0925000 r-xp 00001000 103:05 6977766                   /home/peter/Documents/personalProjects/debugger/build/hello.tsk
    // 5633d0925000-5633d0926000 r--p 00002000 103:05 6977766                   /home/peter/Documents/personalProjects/debugger/build/hello.tsk
    // 5633d0926000-5633d0928000 rw-p 00002000 103:05 6977766                   /home/peter/Documents/personalProjects/debugger/build/hello.tsk

    AssignedMemory(const std::string &procMapsLine);
    const std::string &filePath();
    const std::string &addressStart();
    const std::string &addressEnd();
    bool isReadable();
    bool isWritable();
    bool isExecutable();
    const std::string &offset();

  private:
    std::string d_filePath;
    std::string d_addressStart;
    std::string d_addressEnd;
    std::string d_perms;
    std::string d_offset;
};

class ProcMemMap {
  public:
    void init(pid_t pid);
    std::vector<AssignedMemory> getMapsForFilePath(std::string filePath);

  private:
    std::vector<AssignedMemory> d_assignedMemory;
};

#endif