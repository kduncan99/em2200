//  TDate.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Word36 representing system time and date in ER TDATE$ format



#ifndef     MISCLIB_TDATE_H
#define     MISCLIB_TDATE_H



#include    "Word36.h"



class   TDate : public Word36
{
public:
    TDate()
        :Word36( 0 )
    {}

    TDate( const Word36& word36 )
        :Word36( word36 )
    {}

    TDate( const UINT64 value )
        :Word36( value )
    {}

    TDate( UINT64   month,          //  1=Jan, 2=Feb, et
           UINT64   day,            //  Day of month
           UINT64   year,           //  modulo 64
           UINT64   seconds )       //  since midnight
           :Word36( (month << 30) | (day << 24) | (year << 18) | seconds)
    {}

#ifdef  WIN32
    TDate( const SYSTEMTIME& localTime )
        :Word36( (static_cast<UINT64>(localTime.wMonth) << 30)
                    | (localTime.wDay << 24)
                    | ((localTime.wYear - 1964) << 18)
                    | ((localTime.wHour * 3600) + (localTime.wMinute * 60) + localTime.wSecond) )
    {}
#endif

    inline COUNT32          getMonth() const            { return getS1(); }
    inline COUNT32          getDay() const              { return getS2(); }
    inline COUNT32          getYear() const             { return getS3(); }
    inline COUNT32          getSeconds() const          { return getH2(); }

    inline void             setMonth( COUNT32 month )       { setS1( month ); }
    inline void             setDay( COUNT32 day )           { setS2( day ); }
    inline void             setYear( COUNT32 year )         { setS3( year ); }
    inline void             setSeconds( COUNT32 seconds )   { setH2( seconds ); }
};


std::ostream& operator<<( std::ostream& strm, const TDate& tDate );



#endif
