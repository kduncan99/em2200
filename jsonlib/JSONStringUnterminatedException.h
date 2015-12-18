//  JSONStringUnterminatedException.h
//
//  There is some invalid character in the string being parsed



#ifndef     JSONLIB_JSON_STRING_UNTERMINATED_EXCEPTION_H
#define     JSONLIB_JSON_STRING_UNTERMINATED_EXCEPTION_H



#include    "JSONException.h"



class   JSONStringUnterminatedException : public JSONException
{
public:
    JSONStringUnterminatedException()
        :JSONException( "Unterminated string element" )
    {}
};



#endif