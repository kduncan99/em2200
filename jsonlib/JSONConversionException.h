//  JSONConversionException.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  This is thrown when an attempt to convert from a JSON type to an internal type fails...
//  e.g., conversion of a string containing "FOO" to a boolean value.



#ifndef     JSONLIB_JSON_CONVERSION_EXCEPTION
#define     JSONLIB_JSON_CONVERSION_EXCEPTION



#include    "JSONException.h"



class   JSONConversionException : public JSONException
{
public:
    JSONConversionException()
        :JSONException( "Data conversion error" )
    {}

    JSONConversionException( const char* const pReason)
        :JSONException( pReason )
    {}

    JSONConversionException( const std::string& reason)
        :JSONException( reason )
    {}
};



#endif
