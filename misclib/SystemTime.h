//  SystemTime.h
//  Represents a timestamp for the host system in component form


#ifndef     MISCLIB_SYSTEM_TIME_H
#define     MISCLIB_SYSTEM_TIME_H



class   SystemTime
{
private:
#ifdef WIN32
    SYSTEMTIME      m_Source;
#else
    struct timeval  m_Source;
#endif
    UINT32          m_Day;              //  range 1-31
    UINT32          m_DayOfWeek;        //  range 1-7
    UINT32          m_Hour;             //  range 0-23
    UINT32          m_Microsecond;      //  range 0-999999
    UINT32          m_Minute;           //  range 0-59
    UINT32          m_Month;            //  range 1-12
    UINT32          m_Second;           //  range 0-60 (not a typo)
    UINT32          m_Year;             //  range... whatever
    SystemTime(){}

public:
    static const COUNT64 MILLISECONDS_PER_SECOND = 1000;
    static const COUNT64 MICROSECONDS_PER_SECOND = 1000000;

    inline UINT32 getDay() const            { return m_Day; }                   //  day of the month 1-31
    inline UINT32 getDayOfWeek() const      { return m_DayOfWeek; }             //  day of week 1-7
    inline UINT32 getHour() const           { return m_Hour; }                  //  hour after midnight 0-23
    inline UINT32 getMicrosecond() const    { return m_Microsecond; }           //  microsec since the second
    inline UINT32 getMillisecond() const    { return m_Microsecond / 1000; }    //  millisec since the second
    inline UINT32 getMinute() const         { return m_Minute; }                //  minutes after the hour 0-59
    inline UINT32 getMonth() const          { return m_Month; }                 //  month of the year 1-12
    inline UINT32 getSecond() const         { return m_Second; }                //  seconds after the minute 0-60
    inline UINT32 getYear() const           { return m_Year; }                  //  year

    std::string             getTimeStamp() const;
    COUNT64                 operator-( const SystemTime& subtrahend ) const;

    static SystemTime*      createFromMicroseconds( const COUNT64 microseconds );
    static SystemTime*      createLocalSystemTime();
    static SystemTime*      createUTCSystemTime();
    static COUNT64          getMicrosecondsSinceEpoch();
};


#endif
