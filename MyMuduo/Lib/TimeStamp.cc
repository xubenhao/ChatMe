#include "TimeStamp.h"

static_assert(
    sizeof(TimeStamp) == sizeof(int64_t),          
    "Timestamp is same size as int64_t");

string TimeStamp::toString() const
{
  char buf[32] = {0};
  int64_t seconds = m_nMicroSecondsSinceEpoch 
      / s_nMicroSecondsPerSecond;
  int64_t microseconds = m_nMicroSecondsSinceEpoch 
      % s_nMicroSecondsPerSecond;
  snprintf(
    buf, 
    sizeof(buf), 
    "%" PRId64 ".%06" PRId64 "", 
    seconds, 
    microseconds);
  return buf;
}

// 转化为格式化字符串
string TimeStamp::toFormattedString(
        bool showMicroseconds) const
{
  char buf[64] = {0};
  time_t seconds = (time_t)(m_nMicroSecondsSinceEpoch 
          / s_nMicroSecondsPerSecond);
  struct tm tm_time;
  // 将time_t对象转换为struct tm对象
  gmtime_r(&seconds, &tm_time);

  if (showMicroseconds)
  {
    int microseconds = (int)(m_nMicroSecondsSinceEpoch 
            % s_nMicroSecondsPerSecond);
    snprintf(
        buf, 
        sizeof(buf), 
        "%4d%02d%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900, 
        tm_time.tm_mon + 1, 
        tm_time.tm_mday,
        tm_time.tm_hour, 
        tm_time.tm_min, 
        tm_time.tm_sec,
        microseconds);
  }
  else
  {
    snprintf(
        buf, 
        sizeof(buf), 
        "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, 
        tm_time.tm_mon + 1, 
        tm_time.tm_mday,
        tm_time.tm_hour, 
        tm_time.tm_min, 
        tm_time.tm_sec);
  }

  return buf;
}

TimeStamp TimeStamp::now()
{
  struct timeval tv;
  // 获得struct timeval对象
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  // 将struct timeval对象转化为TimeStamp对象
  return TimeStamp(seconds * s_nMicroSecondsPerSecond + tv.tv_usec);
}


