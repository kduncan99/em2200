//  TDate.cpp
//
//  Some very simple bits dealing specifically with the ER TDATE$ format



#include    "misclib.h"



static const char* MonthTable[] =
{
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};



std::ostream&
operator<<
(
    std::ostream&   strm,
    const TDate&    tDate
)
{
    if ( tDate.isZero() )
        strm << "<0>";
    else
    {
        COUNT value = tDate.getSeconds();
        COUNT seconds = value % 60;
        value /= 60;
        COUNT minutes = value % 60;
        value /= 60;
        COUNT hours = value;

        strm << hours
            << ":" << std::setw( 2 ) << std::setfill( '0' ) << minutes
            << ":" << std::setw( 2 ) << std::setfill( '0' ) << seconds << " ";
        if ( (tDate.getMonth() > 0) && (tDate.getMonth() < 12) )
            strm << MonthTable[tDate.getMonth() - 1];
        else
            strm << "???";
        strm << " " << tDate.getDay() << " " << tDate.getYear() + 1964;
    }

    return strm;
}

