#pragma once
#pragma once
#include <string>
#include <vector>

// Memory partition
struct Partition {
    unsigned int number;
    unsigned int sizeMB;
    std::string code; // "free", "init", or program name
};

// Process control block
struct PCB {
    int pid;
    std::string programName;
    unsigned int partition;
    unsigned int sizeMB;
    std::string state; // "running" or "waiting"
};

// External files (simulated disk)
struct ExternalFile {
    std::string name;
    unsigned int sizeMB;
};

// Function declarations
void initializeSystem();
void simulateFork(int duration);
void simulateExec(const std::string& program, int duration);
void logSystemStatus(const std::string& trace, int time);
