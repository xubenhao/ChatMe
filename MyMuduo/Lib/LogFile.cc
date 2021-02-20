#include "LogFile.h"
#include "FileUtil.h"
#include "ProcessInfo.h"

LogFile::LogFile(
    const string& basename,
    off_t rollSize,
    bool threadSafe,
    int flushInterval,
    int checkEveryN)
  : m_strBaseName(basename),// 日志文件名
    m_nRollSize(rollSize),// 不能超过规定大小 
    m_nFlushInterval(flushInterval),// 刷新间隔－－刷新内容到磁盘
    m_nCheckEveryN(checkEveryN),// 检查
    m_nCount(0),// 计数
    m_nMutex(threadSafe ? new MutexLock : NULL),// 线程安全保证
    m_nStartOfPeriod(0),// 一个时间点
    m_nLastRoll(0),// 一个时间点
    m_nLastFlush(0)// 最后刷新时间
{

  rollFile();// 创建新的日志文件
}

LogFile::~LogFile()
{
}

void LogFile::append(
    const char* logline, // 写日志
    int len)// 内容长度
{
  if (m_nMutex)
  {
    MutexLockGuard lock(*m_nMutex);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (m_nMutex)
  {
    MutexLockGuard lock(*m_nMutex);
    m_nFile->flush();
  }
  else
  {
    m_nFile->flush();
  }
}

void LogFile::append_unlocked(
    const char* logline, 
    int len)
{
  m_nFile->append(logline, len);
  // 写入内容累计到一定量
  // 日志文件内容达到一定量，
  // 后续日志新建日志文件保存
  // 以保证单个日志文件容量不超过指定值
  if (m_nFile->writtenBytes() > m_nRollSize)
  {
    // 做的一件事情
    rollFile();
  }
  else
  {
    // 写入次数
    ++m_nCount;
    // 累计未干嘛写入次数到达一定数量，执行一个操作
    if (m_nCount >= m_nCheckEveryN)
    {
      // 清零计数
      m_nCount = 0;
      // 获取当前时间
      time_t now = ::time(NULL);
      // now / 一天的秒数
      // 把秒数规整为一天秒数的倍数
      time_t _nThisPeriod = 
          now / s_nRollPerSeconds * s_nRollPerSeconds;
      // 两个时间点的比较－－每隔一天新建一个日志文件
      if (_nThisPeriod != m_nStartOfPeriod)
      {
        rollFile();
      }
      // 上次刷新到现在的间隔大于刷新间隔
      // －－每隔一个指定间隔执行一次刷新
      else if (now - m_nLastFlush > m_nFlushInterval)
      {
        // 执行一次刷新
        m_nLastFlush = now;
        m_nFile->flush();
      }
    }
  }
}

bool LogFile::rollFile()
{
  time_t now = 0;
  // 得到日志文件名
  string filename = getLogFileName(
          m_strBaseName, 
          &now);
  // 将秒数规则到一天秒数的倍数
  time_t start = 
      now / s_nRollPerSeconds * s_nRollPerSeconds;
  if (now > m_nLastRoll)
  {
    // 记录时间戳
    m_nLastRoll = now;
    // 记录时间戳
    m_nLastFlush = now;
    // 记录时间戳
    m_nStartOfPeriod = start;
    // 创建新的日志文件 
    m_nFile.reset(new AppendFile(filename));
    return true;
  }

  return false;
}

string LogFile::getLogFileName(
    const string& basename, // 文件名
    time_t* now)// 指向一个time_t对象
{
  string filename;
  // 给字符串指定容量
  filename.reserve(basename.size() + 64);
  // string支持拷贝赋值
  filename = basename;
  char timebuf[32];
  struct tm tm;
  // 当前时间
  *now = time(NULL);
  // 将日历时间，转化为UTC时间
  gmtime_r(now, &tm); 
  // 按指定格式格式化UTC时间为字符串
  strftime(
    timebuf, 
    sizeof(timebuf), 
    ".%Y%m%d-%H%M%S.", 
    &tm);
  // 文件名＋当前时间戳
  filename += timebuf;
  // + 主机名
  filename += hostname();
  char pidbuf[32];
  // 进程ID
  snprintf(
    pidbuf, 
    sizeof(pidbuf), 
    ".%d", 
    pid());
  // +进程ID
  filename += pidbuf;
  // + .log
  filename += ".log";
  return filename;
}


