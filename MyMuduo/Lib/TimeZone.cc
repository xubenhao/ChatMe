#include "TimeZone.h"
#include "Date.h"
    
struct Transition    
{  
    time_t m_nGmtTime;  
    time_t m_nLocalTime;  
    int m_nLocalTimeIdx;  
    Transition(
        time_t t, 
        time_t l, 
        int localIdx)
        : m_nGmtTime(t), 
        m_nLocalTime(l), 
        m_nLocalTimeIdx(localIdx)
    { 
    }    
};
    

struct Comp    
{  
    bool m_bCompareGmt;  
    Comp(bool gmt)
        : m_bCompareGmt(gmt)  
    {  
    }
    
    bool operator()(
        const Transition& lhs, 
        const Transition& rhs) const  
    {
        if (m_bCompareGmt)
        {
            return lhs.m_nGmtTime < rhs.m_nGmtTime;
        }
        else
        {
            return lhs.m_nLocalTime < rhs.m_nLocalTime;
        }  
    }
  
    bool equal(
        const Transition& lhs, 
        const Transition& rhs) const  
    {
        if (m_bCompareGmt)
        {
            return lhs.m_nGmtTime == rhs.m_nGmtTime;
        }
        else
        {
            return lhs.m_nLocalTime == rhs.m_nLocalTime;
        }  
    }    
};
    

struct Localtime    
{  
    time_t m_nGmtOffset;  
    bool m_bIsDst;  
    int m_arrbIdx;  
    Localtime(
        time_t offset, 
        bool dst, 
        int arrb)
        : m_nGmtOffset(offset), 
        m_bIsDst(dst), 
        m_arrbIdx(arrb)
    { }    
};
    
// 将一个seconds转化为一个struct tm对象
inline void fillHMS(
    unsigned seconds, 
    struct tm* utc)    
{  
    utc->tm_sec = seconds % 60;
    unsigned minutes = seconds / 60;
    utc->tm_min = minutes % 60;
    utc->tm_hour = minutes / 60;   
}    


const int s_nSecondsPerDay = 24*60*60;
struct TimeZone::Data
{
  vector<Transition> m_nTransitions;
  vector<Localtime> m_nLocalTimes;
  vector<string> m_vecNames;
  string m_strAbbreviation;
};
        
class File        
{     
public:      
    File(const char* file)    
        : m_pFp(::fopen(file, "rb"))      
    {      
    }
      
    ~File()      
    {    
        if (m_pFp)    
        {
            ::fclose(m_pFp);   
        }      
    }
      
    bool valid() const 
    { 
        return m_pFp; 
    }
      
    string readBytes(int n)      
    {
        char buf[n];
        ssize_t nr = ::fread(buf, 1, n, m_pFp);
        if (nr != n)
        {  
            throw logic_error("no enough data");
        }
            
        return string(buf, n);
    }

    int32_t readInt32()
    {
        int32_t x = 0;
        ssize_t nr = ::fread(&x, 1, sizeof(int32_t), m_pFp);
        if (nr != sizeof(int32_t))
        {
            throw logic_error("bad int32_t data");
        }
            
        return be32toh(x);
    }

          
    uint8_t readUInt8()
    {
        uint8_t x = 0;
        ssize_t nr = ::fread(&x, 1, sizeof(uint8_t), m_pFp);
        if (nr != sizeof(uint8_t))
        {
            throw logic_error("bad uint8_t data");
        }
            
        return x;
    }
         
private:
    FILE* m_pFp;
};

        
bool readTimeZoneFile(
    const char* zonefile, 
    struct TimeZone::Data* data)        
{
    File f(zonefile);
    if (f.valid())
    {
        try
        {
            string head = f.readBytes(4);
            if (head != "TZif")
            {
                throw logic_error("bad head");
            }
              
            string version = f.readBytes(1);
            f.readBytes(15);
            int32_t isgmtcnt = f.readInt32();
            int32_t isstdcnt = f.readInt32();
            int32_t leapcnt = f.readInt32();
            int32_t timecnt = f.readInt32();
            int32_t typecnt = f.readInt32();
            int32_t charcnt = f.readInt32();
            vector<int32_t> trans;
            vector<int> localtimes;
            trans.reserve(timecnt);
            for (int i = 0; i < timecnt; ++i)
            {
                trans.push_back(f.readInt32());
            }

            for (int i = 0; i < timecnt; ++i)
            {
                uint8_t local = f.readUInt8();
                localtimes.push_back(local);
            }
              
            for (int i = 0; i < typecnt; ++i)
            {
                int32_t gmtoff = f.readInt32();
                uint8_t isdst = f.readUInt8();
                uint8_t abbrind = f.readUInt8();
                data->m_nLocalTimes.push_back(
                    Localtime(gmtoff, isdst, abbrind));
            }

            for (int i = 0; i < timecnt; ++i)
            {
                int localIdx = localtimes[i];
                time_t localtime = 
                    trans[i] 
                    + data->m_nLocalTimes[localIdx].m_nGmtOffset;
                data->m_nTransitions.push_back(
                        Transition(trans[i], localtime, localIdx));
            }

            data->m_strAbbreviation = f.readBytes(charcnt);
            //for (int i = 0; i < leapcnt; ++i)
            //{
            //}
              
            //(void) isstdcnt;
            //(void) isgmtcnt;
        }
        catch (logic_error& e)
        {
            fprintf(stderr, "%s\n", e.what());
        }
    }
          
    return true;
}

        
const Localtime* findLocaltime(    
    const TimeZone::Data& data,     
    Transition sentry,     
    Comp comp)        
{
    const Localtime* local = NULL;
    if (data.m_nTransitions.empty() 
        || comp(sentry, data.m_nTransitions.front()))
    {
        local = &data.m_nLocalTimes.front();
    }
    else
    {
        vector<Transition>::const_iterator transI = lower_bound(
                data.m_nTransitions.begin(),
                data.m_nTransitions.end(),
                sentry,
                comp);
            
        if (transI != data.m_nTransitions.end())
        {
            if (!comp.equal(sentry, *transI))
            {
                assert(transI != data.m_nTransitions.begin());
                --transI;
            }

            local = &data.m_nLocalTimes[transI->m_nLocalTimeIdx];
        }
        else
        {
            local = &data.m_nLocalTimes[data.m_nTransitions.back().m_nLocalTimeIdx];
        }
    }

    return local;
}


TimeZone::TimeZone(const char* zonefile)
  : m_pData(new TimeZone::Data)
{
  if (!readTimeZoneFile(zonefile, m_pData.get()))
  {
    m_pData.reset();
  }
}

TimeZone::TimeZone(int eastOfUtc, const char* name)
  : m_pData(new TimeZone::Data)
{
  m_pData->m_nLocalTimes.push_back(
          Localtime(eastOfUtc, false, 0));
  m_pData->m_strAbbreviation = name;
}

struct tm TimeZone::toLocalTime(time_t seconds) const
{
  struct tm localTime;
  memset(&localTime, 0, sizeof(localTime));
  //assert(data_ != NULL);
  const Data& data(*m_pData);
  Transition sentry(seconds, 0, 0);
  const Localtime* local = 
      findLocaltime(
        data, 
        sentry, 
        Comp(true));
  if (local)
  {
    time_t localSeconds = seconds + local->m_nGmtOffset;
    ::gmtime_r(&localSeconds, &localTime);
    localTime.tm_isdst = local->m_bIsDst;
    localTime.tm_gmtoff = local->m_nGmtOffset;
    localTime.tm_zone = &data.m_strAbbreviation[local->m_arrbIdx];
  }

  return localTime;
}

time_t TimeZone::fromLocalTime(
    const struct tm& localTm) const
{
  const Data& data(*m_pData);
  struct tm tmp = localTm;
  time_t seconds = ::timegm(&tmp); 
  Transition sentry(0, seconds, 0);
  const Localtime* local = findLocaltime(
          data, 
          sentry, 
          Comp(false));
  if (localTm.tm_isdst)
  {
    struct tm tryTm = toLocalTime(seconds - local->m_nGmtOffset);
    if (!tryTm.tm_isdst
        && tryTm.tm_hour == localTm.tm_hour
        && tryTm.tm_min == localTm.tm_min)
    {
      seconds -= 3600;
    }
  }

  return seconds - local->m_nGmtOffset;
}

struct tm TimeZone::toUtcTime(
    time_t secondsSinceEpoch, 
    bool yday)
{
  struct tm utc;
  memset(&utc, 0, sizeof(utc));
  utc.tm_zone = "GMT";
  int seconds = (int)(secondsSinceEpoch % s_nSecondsPerDay);
  int days = (int)(secondsSinceEpoch / s_nSecondsPerDay);
  if (seconds < 0)
  {
    seconds += s_nSecondsPerDay;
    --days;
  }

  fillHMS(seconds, &utc);
  Date date(days + Date::s_nDayOf1970_01_01);
  Date::YearMonthDay ymd = date.yearMonthDay();
  utc.tm_year = ymd.year - 1900;
  utc.tm_mon = ymd.month - 1;
  utc.tm_mday = ymd.day;
  utc.tm_wday = date.weekDay();

  if (yday)
  {
    Date startOfYear(
            ymd.year, 
            1, 
            1);
    utc.tm_yday = date.DayNumber() - startOfYear.DayNumber();
  }

  return utc;
}

time_t TimeZone::fromUtcTime(
    const struct tm& utc)
{
  return fromUtcTime(
          utc.tm_year + 1900, 
          utc.tm_mon + 1, 
          utc.tm_mday,
          utc.tm_hour, 
          utc.tm_min, 
          utc.tm_sec);
}

time_t TimeZone::fromUtcTime(
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int seconds)
{
  Date date(year, month, day);
  int secondsInDay = hour * 3600 + minute * 60 + seconds;
  time_t days = date.DayNumber() - Date::s_nDayOf1970_01_01;
  return days * s_nSecondsPerDay + secondsInDay;
}


