#ifndef NLIB_DATE_H
#define NLIB_DATE_H
#include "header.h"
struct tm;
class Date 
{
public:
    struct YearMonthDay
    {
        int year; // [1900..2500]
        int month; // [1..12]
        int day; // [1..31]
    };

    static const int s_nDaysPerWeek = 7;
    static const int s_nDayOf1970_01_01;
    Date()
        : m_nDayNumber(0)
    {
    }

    Date(
        int year, 
        int month, 
        int day);
    explicit Date(int DayNum)
        : m_nDayNumber(DayNum)
    {}
    
    explicit Date(const struct tm&);
    void swap(Date& that)
    {
        std::swap(m_nDayNumber, that.m_nDayNumber);
    }
    
    bool valid() const 
    { 
        return m_nDayNumber > 0; 
    }
    
    std::string toIsoString() const;
    struct YearMonthDay yearMonthDay() const;
    int year() const
    {
        return yearMonthDay().year;
    }

    int month() const
    {
        return yearMonthDay().month;
    }

    int day() const
    {
        return yearMonthDay().day;
    }
    
    int weekDay() const
    {
        return (m_nDayNumber+1) % s_nDaysPerWeek;
    }
    
    int DayNumber() const 
    { 
        return m_nDayNumber; 
    }
    
private:
    int m_nDayNumber;
};

inline bool operator<(Date x, Date y)
{
    return x.DayNumber() < y.DayNumber();
}

inline bool operator==(Date x, Date y)
{
    return x.DayNumber() == y.DayNumber();
}

#endif

