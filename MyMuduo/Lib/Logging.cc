#include "Logging.h"
#include "CurrentThread.h"
#include "TimeStamp.h"
#include "TimeZone.h"

// 每个线程均有一份这些对象
__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(
    int savedErrno)
{
  // 依据错误码，产生对应信息，放入线程全局缓存t_errnobuf
  return strerror_r(
          savedErrno, 
          t_errnobuf, 
          sizeof(t_errnobuf));
}

// 依据环境变量，获取默认级别
Logger::LogLevel initLogLevel()
{
  //if (::getenv("LOG_TRACE"))
  {
    return Logger::TRACE;
  }
  /*else if (::getenv("LOG_DEBUG"))
  {
    return Logger::DEBUG;
  }
  else
  {
    return Logger::INFO;
  }*/
}

Logger::LogLevel g_logLevel = initLogLevel();
// 日志级别对应字符串表述
const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

// 仅在本模板内使用的类
class T
{
 public:
  T(const char* str, unsigned len)
    :m_pStr(str),
     m_nLen(len)
  {
    //assert(strlen(str) == len_);
  }

  const char* m_pStr;
  const unsigned m_nLen;
};

// LogStream?
inline LogStream& operator<<(
    LogStream& s, T v)
{
  s.append(
    v.m_pStr, 
    v.m_nLen);
  return s;
}

// LogStream?
inline LogStream& operator<<(
    LogStream& s, 
    const Logger::SourceFile& v)// 包含文件名信息
{
  s.append(v.m_pData, v.m_nSize);
  return s;
}

// 写信息到标准输出
void defaultOutput(
    const char* msg, 
    int len)
{
  size_t n = fwrite(msg, 1, len, stdout);
  (void)n;
}

// 刷新标准输出
void defaultFlush()
{
  fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
TimeZone g_logTimeZone;

// Logger内部类
Logger::Impl::Impl(
    LogLevel level, // 级别
    int savedErrno, // 错误码
    const SourceFile& file, // 文件名信息
    int line)// 行号
  : m_nTime(TimeStamp::now()),// 时间戳
    
    m_nStream(),// 流对象
    
    m_nLevel(level),
    
    m_nLine(line),
    
    m_nBaseName(file)
{
  formatTime();
  tid();
  
  m_nStream << 
      T(tidString(),// 线程ID 
      tidStringLength());// 线程ID长度
  
  m_nStream << T(
    LogLevelName[level],// 日志级别字符串 
    6);// 长度
  
  if (savedErrno != 0)// 错误码
  {
    m_nStream 
        << strerror_tl(savedErrno) // 错误信息
        << " (errno=" << savedErrno << ") ";// 错误码信息
  }
}

// 时间格式化
void Logger::Impl::formatTime()
{
  // TimeStamp?
  // 返回以毫秒为单位的值
  int64_t microSecondsSinceEpoch = 
      m_nTime.microSecondsSinceEpoch();
  // 转化为以秒为单位的值
  time_t seconds = 
      (time_t)(microSecondsSinceEpoch \
        / TimeStamp::s_nMicroSecondsPerSecond);
  // 余数部分
  int microseconds = 
      (int)(microSecondsSinceEpoch \
        % TimeStamp::s_nMicroSecondsPerSecond);
  // 两次formatTime秒数不一样
  if (seconds != t_lastSecond)
  {
    t_lastSecond = seconds;
    struct tm tm_time;
    // TimeZone? 
    if (g_logTimeZone.valid())
    {
        // 区域转换？
        tm_time = g_logTimeZone.toLocalTime(seconds);
    }
    else
    {
      // 基于标准区域? 
      // 以秒数为单位的日历时间到UTC时间的转化?
      ::gmtime_r(&seconds, &tm_time); 
    }

    // 将时间对象格式化为特定格式的字符串
    int len = snprintf(
        t_time, 
        sizeof(t_time), 
        "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, 
        tm_time.tm_mon + 1, 
        tm_time.tm_mday,
        tm_time.tm_hour, 
        tm_time.tm_min, 
        tm_time.tm_sec);
    //assert(len == 17); 
    (void)len;
  }

  // TimeZone?
  if (g_logTimeZone.valid())
  {
    // 余数部分  
    Fmt us(".%06d ", microseconds);
    //assert(us.length() == 8);
    m_nStream << T(t_time, 17) << T(us.data(), 8);
  }
  else
  {
    Fmt us(".%06dZ ", microseconds);
    //assert(us.length() == 9);
    m_nStream << T(t_time, 17) << T(us.data(), 9);
  }
}

// -文件名:行号\n
void Logger::Impl::finish()
{
  m_nStream 
      << " - " 
      << m_nBaseName 
      << ':' 
      << m_nLine
      << '\n';
}

Logger::Logger(
    SourceFile file, 
    int line)
  : m_nImpl(INFO, 0, file, line)
{
}

Logger::Logger(
    SourceFile file, 
    int line, 
    LogLevel level, 
    const char* func)
  : m_nImpl(level, 0, file, line)
{
  m_nImpl.m_nStream << func << ' ';// 函数名
}

Logger::Logger(
    SourceFile file, 
    int line, 
    LogLevel level)
  : m_nImpl(level, 0, file, line)
{
}

Logger::Logger(
    SourceFile file, 
    int line, 
    bool toAbort)
  : m_nImpl(toAbort ? FATAL : ERROR, errno, file, line)
{
}

Logger::~Logger()
{
  m_nImpl.finish();
  // LogStream?
  // Buffer?
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
  if (m_nImpl.m_nLevel == FATAL)
  {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
  g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz)
{
  g_logTimeZone = tz;
}

