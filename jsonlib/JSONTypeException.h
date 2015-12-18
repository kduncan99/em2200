//  JSONTypeException.h
//
//  Header file for exceptions relating to functions being called on objects which have a type not compatible with the requested function



#ifndef     JSONLIB_JSON_TYPE_EXCEPTION_H
#define     JSONLIB_JSON_TYPE_EXCEPTION_H



#include    "JSONException.h"



class   JSONTypeException : public JSONException
{
public:
    JSONTypeException()
        :JSONException( "Type error" )
    {}

    JSONTypeException( const char* const pReason)
        :JSONException( pReason )
    {}

    JSONTypeException( const std::string& reason)
        :JSONException( reason )
    {}
};



#endif