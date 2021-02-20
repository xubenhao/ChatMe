#ifndef NLIB_TIMEZONE_H
#define NLIB_TIMEZONE_H
#include "header.h"    
class TimeZone
{
public:
    explicit TimeZone(
        const char* zonefile);
    TimeZone(
        int eastOfUtc, 
        const char* tzname);
    TimeZone() = default;
    bool valid() const
    {
        return (bool)(m_pData);
    }
    
    struct tm toLocalTime(
            time_t secondsSinceEpoch) const;
    time_t fromLocalTime(
            const struct tm&) const;
    static struct tm toUtcTime(
        time_t secondsSinceEpoch, 
        bool yday = false);
    static time_t fromUtcTime(
            const struct tm&);
    static time_t fromUtcTime(
        int year,
        int month,
        int day,
        int hour,
        int minute,
        int seconds);
    struct Data;
private:
    std::shared_ptr<Data> m_pData;
};
#endif


