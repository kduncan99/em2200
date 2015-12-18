//  JSONArrayFormatException.h
//
//  We were partially into an array, and hit something we didn't expect



#ifndef     JSONLIB_JSON_ARRAY_FORMAT_EXCEPTION_H
#define     JSONLIB_JSON_ARRAY_FORMAT_EXCEPTION_H



#include    "JSONException.h"



class   JSONArrayFormatException : public JSONException
{
public:
    JSONArrayFormatException()
        :JSONException( "Array incorrectly formatted" )
    {}
};



#endif
