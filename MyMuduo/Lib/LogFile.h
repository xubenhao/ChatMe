#ifndef NLIB_LOGFILE_H
#define NLIB_LOGFILE_H

#include "Mutex.h"
class AppendFile;
class LogFile 
{
public:
    // 单个日志文件不能太大
    // 一段时间后自动将缓存刷新到磁盘文件
    // 每天一个日志
    LogFile(
        const string& basename,
        off_t rollSize,
        bool threadSafe = true,
        int flushInterval = 3,
        int checkEveryN = 1024);
      
    ~LogFile();
    void append(
        const char* logline, 
        int len);
    void flush();
    bool rollFile();
private:
    void append_unlocked(
        const char* logline, 
        int len);
    
    static string getLogFileName(
        const string& basename, 
        time_t* now);
    

private:
    const string m_strBaseName;
    const off_t m_nRollSize;
    const int m_nFlushInterval;
    const int m_nCheckEveryN;
    int m_nCount;
    std::unique_ptr<MutexLock> m_nMutex;
    time_t m_nStartOfPeriod;
    time_t m_nLastRoll;
    time_t m_nLastFlush;
    std::unique_ptr<AppendFile> m_nFile;
    const static int s_nRollPerSeconds = 60*60*24;
};

#endif

