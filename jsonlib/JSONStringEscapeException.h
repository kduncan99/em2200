//  JSONStringEscapeException.h
//
//  Parser expects to find more data, and is at the end of the input text



#ifndef     JSONLIB_JSON_STRING_ESCAPE_EXCEPTION_H
#define     JSONLIB_JSON_STRING_ESCAPE_EXCEPTION_H



#include    "JSONException.h"



class   JSONStringEscapeException : public JSONException
{
public:
    JSONStringEscapeException()
        :JSONException( "Bad escape sequence in string" )
    {}
};



#endif