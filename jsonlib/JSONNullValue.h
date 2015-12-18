//  JSONNullValue.h
//
//  Header file for JSON NULL value



#ifndef     JSONLIB_JSON_NULL_VALUE_H
#define     JSONLIB_JSON_NULL_VALUE_H



#include    "JSONAtomicValue.h"



class   JSONNullValue : public JSONAtomicValue
{
public:
    JSONNullValue()
        :JSONAtomicValue( VT_NULL )
    {}

    //  JSONAtomicValue interface
    std::string                 encode() const;
    std::string                 getValueAsString() const;
    bool                        isNull() const;
};



#endif
