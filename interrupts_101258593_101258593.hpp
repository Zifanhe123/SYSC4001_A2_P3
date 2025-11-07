#pragma once
#include <string>
#include <vector>

// Fixed memory partitions entry
struct Partition {
    unsigned number;       // 1..6
    unsigned sizeMB;       // 40,25,15,10,8,2
    std::string code;      // "free", "init", or program name
};

// Minimal PCB for this simulator
struct PCB {
    int pid;                       // 0 = init
    std::string programName;       // "init" or program name
    int partition;                 // 1..6
    unsigned sizeMB;               // program memory size in MB
    std::string state;             // "running" or "waiting"
};

// Trace step parsed from trace.txt
struct Step {
    std::string op;                // CPU, SYSCALL, END_IO, FORK, EXEC, IF_CHILD, IF_PARENT, ENDIF
    std::vector<std::string> args; // tokens after comma
    int num;                       // numeric argument at end (duration or device)
    std::string raw;               // original line
};

// Interface (implemented in .cpp)
void initializeSystem();
void runTrace(const std::vector<Step>& tr);
