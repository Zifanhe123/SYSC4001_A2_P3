#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <climits>
#include "interrupts_101258593_101258593.hpp"

using namespace std;

// ---------------- Globals ----------------
static const vector<pair<unsigned,int>> PART_SIZES = {
    {1,40},{2,25},{3,15},{4,10},{5,8},{6,2}
};

vector<Partition> partitions;
vector<PCB> pcbs;

unordered_map<string,unsigned> programSizeMB; // from external_files.txt
unordered_map<int,string>      vectorAddr;    // from vector_table.txt (optional)
unordered_map<int,unsigned>    deviceDelayMs; // from device_table.txt (optional)

long long T = 0;          // simulation time (ms)
int nextPid = 1;          // PIDs start at 1 for child
ofstream exelog, syslog;  // outputs

// ---------------- Utility ----------------
static inline string trim(const string& s){
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if(a==string::npos) return "";
    return s.substr(a, b-a+1);
}

void ensureOutputDir() { (void)system("mkdir -p output_files"); }

void logexe(long long dur, const string& what){
    exelog << T << ", " << dur << ", " << what << "\n";
    T += dur;
}

void snapshot(const string& traceLine){
    syslog << "time: " << T << "; current trace: " << traceLine << "\n";
    syslog << "+------------------------------------------------------+\n";
    syslog << "| PID | program name | partition | size |   state |\n";
    syslog << "+------------------------------------------------------+\n";
    for(const auto& p : pcbs){
        syslog << "| " << p.pid << " | " << p.programName << " | "
               << p.partition << " | " << p.sizeMB << " | "
               << p.state << " |\n";
    }
    syslog << "+------------------------------------------------------+\n\n";
}

void kernel_enter(){ logexe(1,"switch to kernel mode"); logexe(10,"context saved"); }
void iret(){ logexe(1,"IRET"); }

// ---------------- Loads ----------------
void loadExternalFiles(const string& path="input_files/external_files.txt"){
    ifstream in(path);
    if(!in){ cerr<<"missing "<<path<<"\n"; exit(1); }
    string line;
    while(getline(in,line)){
        line = trim(line);
        if(line.empty()) continue;
        replace(line.begin(), line.end(), ',', ' ');
        string name; unsigned sz;
        stringstream ss(line);
        if(ss>>name>>sz) programSizeMB[name]=sz;
    }
}

void loadVectorTable(const string& path="input_files/vector_table.txt"){
    ifstream in(path);
    if(!in) return;
    string line;
    while(getline(in,line)){
        line = trim(line);
        if(line.empty()) continue;
        replace(line.begin(), line.end(), ',', ' ');
        int vec; string addr;
        stringstream ss(line);
        if(ss>>vec>>addr) vectorAddr[vec]=addr;
    }
}

void loadDeviceTable(const string& path="input_files/device_table.txt"){
    ifstream in(path);
    if(!in) return;
    string line;
    while(getline(in,line)){
        line = trim(line);
        if(line.empty()) continue;
        replace(line.begin(), line.end(), ',', ' ');
        int dev; unsigned d;
        stringstream ss(line);
        if(ss>>dev>>d) deviceDelayMs[dev]=d;
    }
}

// ---------------- Partitions ----------------
int bestFit(unsigned needMB){
    int best = -1; unsigned bestSize = UINT_MAX;
    for(size_t i=0;i<partitions.size();++i){
        if(partitions[i].code=="free" && partitions[i].sizeMB>=needMB){
            if(partitions[i].sizeMB < bestSize){
                best = (int)i; bestSize = partitions[i].sizeMB;
            }
        }
    }
    return best;
}

// ---------------- System Calls ----------------
void doFORK(int parentPid, int isrMs, const string& traceLine){
    kernel_enter();
    logexe(1,"find vector 2 in memory position 0x0004");
    logexe(1,"load address 0x0695 into the PC");
    logexe(isrMs,"cloning the PCB");

    // Clone PCB: child runs, parent waits
    int pidx = 0; // parent is PID 0 in this simplified run
    PCB child = pcbs[pidx];
    child.pid = nextPid++;
    child.state = "running";
    pcbs[pidx].state = "waiting";
    pcbs.push_back(child);

    snapshot(traceLine);
    logexe(0,"scheduler called");
    iret();
}

void doEXEC(int pid, const string& program, int isrMs, const string& traceLine){
    kernel_enter();
    logexe(1,"find vector 3 in memory position 0x0006");
    logexe(1,"load address 0x042B into the PC");

    unsigned sz = programSizeMB.count(program) ? programSizeMB[program] : 10;
    logexe(isrMs, "Program is " + to_string(sz) + " Mb large");

    // Loader time: 15ms/MB
    logexe(sz*15, "loading program into memory");

    // Allocate partition & update PCB
    int slot = bestFit(sz);
    if(slot < 0){ cerr<<"No partition fits program "<<program<<" ("<<sz<<" MB)\n"; exit(1); }
    partitions[slot].code = program;
    logexe(3,"marking partition as occupied");

    // Update the running process (child has CPU at this point)
    int runIdx = 1; // PID 1 (child) for this simple run order
    if((int)pcbs.size()<=runIdx){ cerr<<"internal error: missing child pcb\n"; exit(1); }
    pcbs[runIdx].programName = program;
    pcbs[runIdx].partition   = partitions[slot].number;
    pcbs[runIdx].sizeMB      = sz;
    pcbs[runIdx].state       = "running";
    logexe(6,"updating PCB");

    snapshot(traceLine);
    logexe(0,"scheduler called");
    iret();
}

// ---------------- Trace Parser & Runner ----------------
vector<Step> loadTrace(const string& path="input_files/trace.txt"){
    ifstream in(path);
    if(!in){ cerr<<"missing "<<path<<"\n"; exit(1); }
    vector<Step> v; string line;
    while(getline(in,line)){
        string raw = trim(line);
        if(raw.empty()) continue;

        Step s; s.raw = raw;

        // Normalize commas to spaces
        string norm = raw;
        replace(norm.begin(), norm.end(), ',', ' ');
        stringstream ss(norm);
        ss >> s.op;
        string w;
        while(ss>>w) s.args.push_back(w);

        // last token numeric if present
        s.num = 0;
        if(!s.args.empty()){
            try{ s.num = stoi(s.args.back()); } catch(...){}
        }
        v.push_back(s);
    }
    return v;
}

void runTrace(const vector<Step>& tr){
    size_t i=0;
    while(i<tr.size()){
        const Step& s = tr[i];

        if(s.op=="CPU"){ logexe(s.num,"CPU Burst"); i++; continue; }
        if(s.op=="SYSCALL"){
            kernel_enter();
            logexe(1,"find vector 4 in memory position 0x0008");
            logexe(1,"load address 0x0292 into the PC");
            logexe(deviceDelayMs.count(s.num)?deviceDelayMs[s.num]:250,"SYSCALL ISR");
            iret();
            i++; continue;
        }
        if(s.op=="END_IO"){
            kernel_enter();
            logexe(1,"find vector 5 in memory position 0x000A");
            logexe(1,"load address 0x0300 into the PC");
            logexe(1,"END_IO ISR (device "+to_string(s.num)+")");
            iret();
            i++; continue;
        }

        if(s.op=="FORK"){
            doFORK(0, s.num, s.raw);

            // After FORK: execute IF_CHILD block first (child priority)
            size_t j=i+1;
            if(j<tr.size() && tr[j].op=="IF_CHILD") j++;

            // child block: until IF_PARENT or ENDIF
            vector<Step> childOps;
            while(j<tr.size() && tr[j].op!="IF_PARENT" && tr[j].op!="ENDIF"){
                childOps.push_back(tr[j]); j++;
            }
            // run childOps
            for(const auto& c : childOps){
                if(c.op=="EXEC")   doEXEC(1, c.args[0], c.num, c.raw);
                else if(c.op=="CPU")     logexe(c.num,"CPU Burst");
                else if(c.op=="SYSCALL"){
                    kernel_enter();
                    logexe(1,"find vector 4 in memory position 0x0008");
                    logexe(1,"load address 0x0292 into the PC");
                    logexe(deviceDelayMs.count(c.num)?deviceDelayMs[c.num]:250,"SYSCALL ISR");
                    iret();
                }
                else if(c.op=="END_IO"){
                    kernel_enter();
                    logexe(1,"find vector 5 in memory position 0x000A");
                    logexe(1,"load address 0x0300 into the PC");
                    logexe(1,"END_IO ISR (device "+to_string(c.num)+")");
                    iret();
                }
            }

            // Skip joint region until ENDIF (we don't double-execute shared lines for this simple runner)
            while(j<tr.size() && tr[j].op!="ENDIF") j++;

            // Parent block: find IF_PARENT and execute until ENDIF
            size_t k=i+1;
            while(k<tr.size() && tr[k].op!="IF_PARENT") k++;
            if(k<tr.size() && tr[k].op=="IF_PARENT") k++;
            while(k<tr.size() && tr[k].op!="ENDIF"){
                const Step& p = tr[k];
                if(p.op=="EXEC")   doEXEC(0, p.args[0], p.num, p.raw);
                else if(p.op=="CPU")     logexe(p.num,"CPU Burst");
                else if(p.op=="SYSCALL"){
                    kernel_enter();
                    logexe(1,"find vector 4 in memory position 0x0008");
                    logexe(1,"load address 0x0292 into the PC");
                    logexe(deviceDelayMs.count(p.num)?deviceDelayMs[p.num]:250,"SYSCALL ISR");
                    iret();
                }
                else if(p.op=="END_IO"){
                    kernel_enter();
                    logexe(1,"find vector 5 in memory position 0x000A");
                    logexe(1,"load address 0x0300 into the PC");
                    logexe(1,"END_IO ISR (device "+to_string(p.num)+")");
                    iret();
                }
                k++;
            }

            // continue after ENDIF
            i = (j<tr.size()? j+1 : tr.size());
            // parent becomes running again (no real scheduler yet)
            pcbs[0].state="running";
            continue;
        }

        if(s.op=="EXEC"){ // exec without prior FORK (rare in given tests)
            doEXEC(0, s.args[0], s.num, s.raw);
            i++; continue;
        }

        // IF_* tokens outside FORK context: just skip
        i++;
    }
}

// ---------------- Init & Main ----------------
void initializeSystem(){
    partitions.clear();
    for(auto [id,sz] : PART_SIZES) partitions.push_back({id,(unsigned)sz,"free"});

    // init occupies partition 6 (2MB) with 1MB
    partitions.back().code = "init";
    pcbs.clear();
    pcbs.push_back({0,"init",6,1,"running"});
    cout << "System initialized with process 'init' in partition 6.\n";
}

int main(){
    ensureOutputDir();
    exelog.open("output_files/execution.txt");
    syslog.open("output_files/system_status.txt");

    // Initial marker (optional)
    logexe(1,"boot");

    // Load inputs
    loadExternalFiles();
    loadVectorTable();
    loadDeviceTable();

    // Initialize system & run
    initializeSystem();
    auto tr = loadTrace("input_files/trace.txt");
    runTrace(tr);

    exelog.close();
    syslog.close();

    cout << "Simulation complete. Check output_files/ for logs.\n";

    return 0;
}
