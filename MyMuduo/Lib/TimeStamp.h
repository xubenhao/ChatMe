#ifndef NLIB_TIMESTAMP_H
#define NLIB_TIMESTAMP_H
#include "header.h"
class TimeStamp :
    public boost::equality_comparable<TimeStamp>,
    public boost::less_than_comparable<TimeStamp>
{
public:
    TimeStamp()
        : m_nMicroSecondsSinceEpoch(0)
    {
    }

    explicit TimeStamp(
            int64_t microSecondsSinceEpochArg)
        : m_nMicroSecondsSinceEpoch(microSecondsSinceEpochArg)
    {
    
    }

    // 交换
    void swap(TimeStamp& that)
    {
        std::swap(
            m_nMicroSecondsSinceEpoch, 
            that.m_nMicroSecondsSinceEpoch);
    }
    
    string toString() const;
    // 格式化显示
    string toFormattedString(
            bool showMicroseconds = true) const;
    bool valid() const 
    { 
        return m_nMicroSecondsSinceEpoch > 0; 
    }
    
    // 获取值
    int64_t microSecondsSinceEpoch() const 
    { 
        return m_nMicroSecondsSinceEpoch; 
    }
    
    // 转换为以秒为单位的值
    time_t secondsSinceEpoch() const
    { 
        return (time_t)(m_nMicroSecondsSinceEpoch
                / s_nMicroSecondsPerSecond); 
    }

    static TimeStamp now();
    static TimeStamp invalid()
    {
        return TimeStamp();
    }

    // 从time_t对象得到TimeStamp对象
    static TimeStamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0);
    }
    
    static TimeStamp fromUnixTime(
            time_t t, 
            int microseconds)
    {
        return 
            TimeStamp((int64_t)(t) 
                    * s_nMicroSecondsPerSecond 
                + microseconds);
    }


    
    static const int s_nMicroSecondsPerSecond 
        = 1000 * 1000;
private:
    int64_t m_nMicroSecondsSinceEpoch;
};

inline bool operator<(
        TimeStamp lhs, 
        TimeStamp rhs)
{
    return lhs.microSecondsSinceEpoch() 
        < rhs.microSecondsSinceEpoch();
}

inline bool operator==(
        TimeStamp lhs, 
        TimeStamp rhs)
{
    return lhs.microSecondsSinceEpoch() 
        == rhs.microSecondsSinceEpoch();
}

// 以秒为单位的差值
inline double timeDifference(
        TimeStamp high, 
        TimeStamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() 
        - low.microSecondsSinceEpoch();
    return (double)(diff) 
        / TimeStamp::s_nMicroSecondsPerSecond;
}

inline TimeStamp addTime(
        TimeStamp timestamp, 
        double seconds)
{
    int64_t delta = (int64_t)(seconds 
            * TimeStamp::s_nMicroSecondsPerSecond);
    return TimeStamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif


