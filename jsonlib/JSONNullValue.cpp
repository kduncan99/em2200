//  JSONNullValue.cpp
//
//  Implementation of JSONNullValue class



#include    "jsonlib.h"



//  encode()
//
//  Serializes the value of this object
std::string
JSONNullValue::encode() const
{
    return "null";
}


//  getValueAsString
std::string
JSONNullValue::getValueAsString() const
{
    return "null";
}


//  isNull()
//
//  Indicates whether this object has no value
bool
JSONNullValue::isNull() const
{
    return true;
}

