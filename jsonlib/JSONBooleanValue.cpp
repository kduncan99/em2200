//  JSONBooleanValue.cpp
//
//  Implementation of JSONBooleanValue class



#include    "jsonlib.h"



//  encode()
//
//  Serializes the value of this object
std::string
JSONBooleanValue::encode() const
{
    return m_Value ? "true" : "false";
}


//  getValueAsBool()
//
//  Interprets the value of this object as a boolean
bool
JSONBooleanValue::getValueAsBool() const
{
    return m_Value;
}


//  getValueAsSigned32()
//
//  Interprets the value of this object as an integer
long int
JSONBooleanValue::getValueAsSigned32() const
{
    return m_Value ? 1 : 0;
}


//  getValueAsSigned64()
//
//  Interprets the value of this object as an integer
long long int
JSONBooleanValue::getValueAsSigned64() const
{
    return m_Value ? 1 : 0;
}


//  getValueAsString()
//
//  Interprets the value of this object as a string
std::string
JSONBooleanValue::getValueAsString() const
{
    return m_Value ? "true" : "false";
}


//  getValueAsUnsigned32()
//
//  Interprets the value of this object as an integer
unsigned long
JSONBooleanValue::getValueAsUnsigned32() const
{
    return m_Value ? 1 : 0;
}


//  getValueAsUnsigned64()
//
//  Interprets the value of this object as an integer
unsigned long long
JSONBooleanValue::getValueAsUnsigned64() const
{
    return m_Value ? 1 : 0;
}


//  isNull()
//
//  Indicates whether this object has no value
bool
JSONBooleanValue::isNull() const
{
    return false;
}


//  isZero()
//
//  Interprets this object's value as an integer, and indicates whether that integer is zero
bool
JSONBooleanValue::isZero() const
{
    return !m_Value;
}

