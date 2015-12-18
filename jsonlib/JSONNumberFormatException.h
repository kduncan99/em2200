//  JSONNumberFormatException.h
//
//  We were partially into an number, and hit something we didn't expect



#ifndef     JSONLIB_JSON_NUMBER_FORMAT_EXCEPTION_H
#define     JSONLIB_JSON_NUMBER_FORMAT_EXCEPTION_H



#include    "JSONException.h"



class   JSONNumberFormatException : public JSONException
{
public:
    JSONNumberFormatException()
        :JSONException( "Number incorrectly formatted" )
    {}
};



#endif
