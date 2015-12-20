//  SystemTime generally useful object
//  Copyright (c) 2015 by Kurt Duncan



#include    "misclib.h"



//  Local constants

//  Windows epoch for FILETIME structure is Jan 1 1601 00:00:00
//  Linux epoch for timeval structure is Jan 1 1970 00:00:00
//  2200 DWTIME$ epoch is Dec 31 1899 00:00:00
//  There are 11,643,609,600 seconds difference between Windows and Linux epochs
//             9,434,534,400 seconds difference from Windows to 2200 epoch
//             2,209,075,200 seconds difference from 2200 to Linux epoch
#ifdef WIN32
#define     WINDOWS_EPOCH_OFFSET_SECONDS            9434534400LL
#else
#define     LINUX_EPOCH_OFFSET_SECONDS              2209075200LL
#endif



//  public functions

//  getTimeStamp()
//
//  Returns a string representing this object's time, suitable for use in a log file name
std::string
SystemTime::getTimeStamp() const
{
    std::stringstream strm;
    strm << getYear()
        << std::setfill( '0' ) << std::setw( 2 ) << getMonth()
        << std::setfill( '0' ) << std::setw( 2 ) << getDay()
        << "-" << std::setfill( '0' ) << std::setw( 2 ) << getHour()
        << std::setfill( '0' ) << std::setw( 2 ) << getMinute()
        << std::setfill( '0' ) << std::setw( 2 ) << getSecond();
    return strm.str();
}


//  operator-()
COUNT64
SystemTime::operator -
(
const SystemTime& subtrahend
) const
{
#ifdef WIN32
    FILETIME ourFileTime;  //  units of 100nanosec since epoch
    SystemTimeToFileTime( &m_Source, &ourFileTime );
    COUNT64 ourTemp = (COUNT64)ourFileTime.dwHighDateTime << 32 | ourFileTime.dwLowDateTime;
    COUNT64 ourMicros = ourTemp / 10;           //  convert 100nanosec to 1microsec

    FILETIME subtraFileTime;
    SystemTimeToFileTime( &subtrahend.m_Source, &subtraFileTime );
    COUNT64 subtraTemp = (COUNT64)subtraFileTime.dwHighDateTime << 32 | subtraFileTime.dwLowDateTime;
    COUNT64 subtraMicros = subtraTemp / 10;     //  convert 100nanosec to 1microsec

    return ourMicros - subtraMicros;
#else
    COUNT64 ourMicros = (COUNT64) m_Source.tv_sec * MICROSECONDS_PER_SECOND;
    ourMicros += m_Source.tv_usec;

    COUNT64 subtraMicros = (COUNT64) subtrahend.m_Source.tv_sec * MICROSECONDS_PER_SECOND;
    subtraMicros += subtrahend.m_Source.tv_usec;

    return ourMicros - subtraMicros;
#endif
}



//  public static functions

//  createFromMicroseconds()
//
//  Creates a new object representing the indicated time in microseconds since OS2200 epoch
SystemTime*
SystemTime::createFromMicroseconds
(
const COUNT64       microseconds
)
{
    SystemTime* pSysTime = new SystemTime();

#ifdef WIN32
    COUNT64 winMicros = microseconds + ( WINDOWS_EPOCH_OFFSET_SECONDS * MICROSECONDS_PER_SECOND );

    FILETIME fileTime;
    fileTime.dwHighDateTime = winMicros >> 32;
    fileTime.dwLowDateTime = winMicros & 0xFFFFFFFF;
    FileTimeToSystemTime( &fileTime, &pSysTime->m_Source );

    pSysTime->m_Year = pSysTime->m_Source.wYear;
    pSysTime->m_Month = pSysTime->m_Source.wMonth;
    pSysTime->m_Day = pSysTime->m_Source.wDay;
    pSysTime->m_DayOfWeek = pSysTime->m_Source.wDayOfWeek + 1;
    pSysTime->m_Hour = pSysTime->m_Source.wHour;
    pSysTime->m_Minute = pSysTime->m_Source.wMinute;
    pSysTime->m_Second = pSysTime->m_Source.wSecond;
    pSysTime->m_Microsecond = pSysTime->m_Source.wMilliseconds * 1000;
#else
    COUNT64 linMicros = microseconds - ( LINUX_EPOCH_OFFSET_SECONDS * MICROSECONDS_PER_SECOND );
    pSysTime->m_Source.tv_sec = linMicros / MICROSECONDS_PER_SECOND;
    pSysTime->m_Source.tv_usec = linMicros % MICROSECONDS_PER_SECOND;

    struct tm tmStruct;
    gmtime_r( &pSysTime->m_Source.tv_sec, &tmStruct );

    pSysTime->m_Year = tmStruct.tm_year + 1900;
    pSysTime->m_Month = tmStruct.tm_mon + 1;
    pSysTime->m_Day = tmStruct.tm_mday;
    pSysTime->m_DayOfWeek = tmStruct.tm_wday + 1;
    pSysTime->m_Hour = tmStruct.tm_hour;
    pSysTime->m_Minute = tmStruct.tm_min;
    pSysTime->m_Second = tmStruct.tm_sec;
    pSysTime->m_Microsecond = pSysTime->m_Source.tv_usec;
#endif

    return pSysTime;
}


//  createLocalSystemTime()
//
//  Creates a new object representing 'now' in local time
SystemTime*
SystemTime::createLocalSystemTime()
{
    SystemTime* pSysTime = new SystemTime();

#ifdef WIN32
    GetLocalTime( &pSysTime->m_Source );

    pSysTime->m_Year = pSysTime->m_Source.wYear;
    pSysTime->m_Month = pSysTime->m_Source.wMonth;
    pSysTime->m_Day = pSysTime->m_Source.wDay;
    pSysTime->m_DayOfWeek = pSysTime->m_Source.wDayOfWeek + 1;
    pSysTime->m_Hour = pSysTime->m_Source.wHour;
    pSysTime->m_Minute = pSysTime->m_Source.wMinute;
    pSysTime->m_Second = pSysTime->m_Source.wSecond;
    pSysTime->m_Microsecond = pSysTime->m_Source.wMilliseconds * 1000;
#else
    gettimeofday( &pSysTime->m_Source, 0 );

    struct tm tmStruct;
    localtime_r( &pSysTime->m_Source.tv_sec, &tmStruct );

    pSysTime->m_Year = tmStruct.tm_year + 1900;
    pSysTime->m_Month = tmStruct.tm_mon + 1;
    pSysTime->m_Day = tmStruct.tm_mday;
    pSysTime->m_DayOfWeek = tmStruct.tm_wday + 1;
    pSysTime->m_Hour = tmStruct.tm_hour;
    pSysTime->m_Minute = tmStruct.tm_min;
    pSysTime->m_Second = tmStruct.tm_sec;
    pSysTime->m_Microsecond = pSysTime->m_Source.tv_usec;
#endif

    return pSysTime;
}


//  createUTCSystemTime()
//
//  Creates a new object representing 'now' in UTC time
SystemTime*
SystemTime::createUTCSystemTime()
{
    SystemTime* pSysTime = new SystemTime();

#ifdef WIN32
    GetSystemTime(&pSysTime->m_Source);

    pSysTime->m_Year = pSysTime->m_Source.wYear;
    pSysTime->m_Month = pSysTime->m_Source.wMonth;
    pSysTime->m_Day = pSysTime->m_Source.wDay;
    pSysTime->m_DayOfWeek = pSysTime->m_Source.wDayOfWeek + 1;
    pSysTime->m_Hour = pSysTime->m_Source.wHour;
    pSysTime->m_Minute = pSysTime->m_Source.wMinute;
    pSysTime->m_Second = pSysTime->m_Source.wSecond;
    pSysTime->m_Microsecond = pSysTime->m_Source.wMilliseconds * 1000;
#else
    gettimeofday( &pSysTime->m_Source, 0 );

    struct tm tmStruct;
    gmtime_r( &pSysTime->m_Source.tv_sec, &tmStruct );

    pSysTime->m_Year = tmStruct.tm_year + 1900;
    pSysTime->m_Month = tmStruct.tm_mon + 1;
    pSysTime->m_Day = tmStruct.tm_mday;
    pSysTime->m_DayOfWeek = tmStruct.tm_wday + 1;
    pSysTime->m_Hour = tmStruct.tm_hour;
    pSysTime->m_Minute = tmStruct.tm_min;
    pSysTime->m_Second = tmStruct.tm_sec;
    pSysTime->m_Microsecond = pSysTime->m_Source.tv_usec;
#endif

    return pSysTime;
}


//  getMicrosecondsSinceEpoch()
//
//  Retrieves the represented time as the number of usec which have elapsed since the 2200 epoch (in UTC)
//  Windows epoch for FILETIME structure is Jan 1 1601 00:00:00
//  Linux epoch for timeval structure is Jan 1 1970 00:00:00
//  2200 DWTIME$ epoch is Dec 31 1899 00:00:00
//  There are 11,643,609,600 seconds difference between Windows and Linux epochs
//             9,434,534,400 seconds difference from Windows to 2200 epoch
//             2,209,075,200 seconds difference from 2200 to Linux epoch
COUNT64
SystemTime::getMicrosecondsSinceEpoch()
{
#ifdef WIN32
    SYSTEMTIME systemTime;
    FILETIME fileTime;      //  units of 100 nanoseconds since windows epoch
    GetSystemTime( &systemTime );
    SystemTimeToFileTime( &systemTime, &fileTime );
    COUNT64 temp = (COUNT64)fileTime.dwHighDateTime << 32 | fileTime.dwLowDateTime;
    COUNT64 usec = temp / 10;   //  convert 100nanosec to 1microsec
    usec -= WINDOWS_EPOCH_OFFSET_SECONDS * MICROSECONDS_PER_SECOND;
    return usec;
#else
    struct timeval currentTime;
    gettimeofday( &currentTime, 0 );
    COUNT64 usec = (COUNT64)currentTime.tv_sec * MICROSECONDS_PER_SECOND;
    usec += currentTime.tv_usec;
    usec += LINUX_EPOCH_OFFSET_SECONDS * MICROSECONDS_PER_SECOND;
    return usec;
#endif
}

