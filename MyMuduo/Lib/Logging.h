#ifndef NLIB_LOGGING_H
#define NLIB_LOGGING_H
#include "LogStream.h"
#include "TimeStamp.h"
    
class TimeZone;
class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };
    
    // 类
    class SourceFile
    {
    public:
        // 类成员函数是模板函数
        template<int N>
        SourceFile(const char (&arr_)[N])
        : m_pData(arr_),
          m_nSize(N-1)// 有效字符个数
        {
            // 寻找子串
            const char* slash = strrchr(m_pData, '/'); 
            // 
            if (slash)
            {
              // 移动到/字符下一字符位置
              m_pData = slash + 1;
              m_nSize -= (int)(m_pData - arr_);
            }
        }

        // m_pData只保存文件名，不含路径信息
        explicit SourceFile(
            const char* filename)
            : m_pData(filename)
        {
            const char* slash = strrchr(filename, '/');
            if (slash)
            {
              m_pData = slash + 1;
            }

            m_nSize = (int)(strlen(m_pData));
        }
        
        const char* m_pData;
        int m_nSize;
    };

    class Impl
    {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, 
            int old_errno, 
            const SourceFile& file, 
            int line);
        void formatTime();
        void finish();
        TimeStamp m_nTime;
        LogStream m_nStream;
        LogLevel m_nLevel;
        int m_nLine;
        SourceFile m_nBaseName;
    };

public:
    
    Logger(
        SourceFile file, 
        int line);
    Logger(
        SourceFile file, 
        int line, 
        LogLevel level);
    Logger(
        SourceFile file, 
        int line, 
        LogLevel level, 
        const char* func);
    Logger(
        SourceFile file, 
        int line, 
        bool toAbort);
    ~Logger();

    LogStream& stream() 
    { 
        return m_nImpl.m_nStream; 
    }

    static LogLevel logLevel();
    static void setLogLevel(
            LogLevel level);
    typedef void (*OutputFunc)(
            const char* msg, 
            int len);
    typedef void (*FlushFunc)();
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);
    static void setTimeZone(const TimeZone& tz);
private:
    Impl m_nImpl;
};


extern Logger::LogLevel g_logLevel;
inline Logger::LogLevel Logger::logLevel()
{
    return g_logLevel;
}

#define LOG_TRACE if(g_logLevel >= Logger::TRACE)\
        Logger(\
        __FILE__, \
        __LINE__,\
        Logger::TRACE,\
        __func__).stream()

#define LOG_DEBUG if(g_logLevel >= Logger::DEBUG)\
        Logger(\
        __FILE__, \
        __LINE__, \
        Logger::DEBUG, \
        __func__).stream()

#define LOG_INFO if(g_logLevel >= Logger::INFO)\
        Logger(\
        __FILE__, \
        __LINE__).stream()

#define LOG_WARN if(g_logLevel >= Logger::WARN)\
        Logger(\
        __FILE__, \
        __LINE__, \
        Logger::WARN).stream()

#define LOG_ERROR if(g_logLevel >= Logger::ERROR)\
        Logger(\
        __FILE__, \
        __LINE__, \
        Logger::ERROR).stream()

#define LOG_FATAL if(g_logLevel >= Logger::FATAL)\
        Logger(\
        __FILE__, \
        __LINE__, \
        Logger::FATAL).stream()

#define LOG_SYSERR Logger(\
        __FILE__, \
        __LINE__, \
        false).stream()

#define LOG_SYSFATAL Logger(\
        __FILE__, \
        __LINE__, \
        true).stream()

const char* strerror_tl(int savedErrno);    
#define CHECK_NOTNULL(val) \
    CheckNotNull(\
        __FILE__, \
        __LINE__, \
        "'" #val "' Must be non NULL", \
        (val))
    
template <typename T>
T* CheckNotNull(
    Logger::SourceFile file, 
    int line, 
    const char *names, 
    T* ptr)
{
    if (ptr == NULL)   
    {
        Logger(
            file, 
            line, 
            Logger::FATAL).stream() << names; 
    }
    
    return ptr;
}

#endif

