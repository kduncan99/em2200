//  JSONStringContentException.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  There is some invalid character in the string being parsed



#ifndef     JSONLIB_JSON_STRING_CONTENT_EXCEPTION_H
#define     JSONLIB_JSON_STRING_CONTENT_EXCEPTION_H



#include    "JSONException.h"



class   JSONStringContentException : public JSONException
{
public:
    JSONStringContentException()
        :JSONException( "Bad content in parsed string" )
    {}
};



#endif