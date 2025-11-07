#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "interrupts_101258593_101258593.hpp"

using namespace std;

vector<Partition> partitions;
vector<PCB> pcbs;
vector<ExternalFile> externalFiles;

void initializeSystem() {
    partitions = {
        {1, 40, "free"},
        {2, 25, "free"},
        {3, 15, "free"},
        {4, 10, "free"},
        {5, 8,  "free"},
        {6, 2,  "init"}
    };

    PCB init{0, "init", 6, 1, "running"};
    pcbs.push_back(init);
    cout << "System initialized with process 'init' in partition 6.\n";
}

void simulateFork(int duration) {
    cout << "[FORK] Duration: " << duration << " ms\n";
    PCB parent = pcbs[0];
    PCB child = parent;
    child.pid = 1;
    child.state = "running";
    parent.state = "waiting";
    pcbs[0] = parent;
    pcbs.push_back(child);
    cout << "Created child process PID=1 from parent PID=0.\n";
}

void simulateExec(const string& program, int duration) {
    cout << "[EXEC] Program: " << program 
         << ", Duration: " << duration << " ms\n";
    cout << "Simulating program load and memory assignment...\n";
}

void logSystemStatus(const string& trace, int time) {
    ofstream out("output_files/system_status.txt", ios::app);
    out << "time: " << time << "; current trace: " << trace << "\n";
    out << "+------------------------------------------------------+\n";
    out << "| PID | program name | partition | size | state |\n";
    out << "+------------------------------------------------------+\n";
    for (auto& p : pcbs) {
        out << "| " << p.pid << " | " << p.programName << " | "
            << p.partition << " | " << p.sizeMB << " | "
            << p.state << " |\n";
    }
    out << "+------------------------------------------------------+\n\n";
    out.close();
}

int main() {
    (void)system("mkdir -p output_files");
    ofstream execOut("output_files/execution.txt");
    execOut << "0,1,switch to kernel mode\n";
    execOut.close();

    initializeSystem();
    simulateFork(10);
    simulateExec("program1", 50);
    logSystemStatus("EXEC program1,50", 50);

    cout << "Simulation complete. Check output_files/ for logs.\n";
    return 0;
}
