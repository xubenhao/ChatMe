#ifndef NLIB_PROCESSINFO_H
#define NLIB_PROCESSINFO_H
#include "header.h"
#include "StringPiece.h"
#include "TimeStamp.h"

pid_t pid();
string pidString();
uid_t uid();
string username();
uid_t euid();
TimeStamp startTime();
int clockTicksPerSecond();
int pageSize();
bool isDebugBuild();  
string hostname();
string procname();
StringPiece procname(const string& stat);
string procStatus();
string procStat();
string threadStat();
string exePath();
int openedFiles();
int maxOpenFiles();

struct CpuTime
{
    double userSeconds;
    double systemSeconds;
    CpuTime() 
        : userSeconds(0.0), 
        systemSeconds(0.0) 
    { }

    double total() const 
    { 
        return userSeconds 
            + systemSeconds; 
    }
    
};

CpuTime cpuTime();
int numThreads();
std::vector<pid_t> threads();
#endif

