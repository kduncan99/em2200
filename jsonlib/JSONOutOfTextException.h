//  JSONOutOfTextException.h
//
//  Parser expects to find more data, and is at the end of the input text



#ifndef     JSONLIB_JSON_OUT_OF_TEXT_EXCEPTION_H
#define     JSONLIB_JSON_OUT_OF_TEXT_EXCEPTION_H



#include    "JSONException.h"



class   JSONOutOfTextException : public JSONException
{
public:
    JSONOutOfTextException()
        :JSONException( "Out of text" )
    {}
};



#endif