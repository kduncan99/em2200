//  JSONObjectFormatException.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  We were partially into an object, and hit something we didn't expect



#ifndef     JSONLIB_JSON_OBJECT_FORMAT_EXCEPTION_H
#define     JSONLIB_JSON_OBJECT_FORMAT_EXCEPTION_H



#include    "JSONException.h"



class   JSONObjectFormatException : public JSONException
{
public:
    JSONObjectFormatException()
        :JSONException( "Object incorrectly formatted" )
    {}
};



#endif
