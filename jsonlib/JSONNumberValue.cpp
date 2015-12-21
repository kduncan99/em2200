//  JSONNumberValue.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implements the JSONNumberValue class



#include    "jsonlib.h"



//  encode()
//
//  Serializes the value of this object
std::string
JSONNumberValue::encode() const
{
    return m_Value;
}


//  getValueAsBool()
//
//  Interprets the value of this object as a boolean
bool
JSONNumberValue::getValueAsBool() const
{
    return !isZero();
}


//  getValueAsSigned32()
//
//  Interprets the value of this object as an integer
long int
JSONNumberValue::getValueAsSigned32() const
{
    long result = 0;
    bool negative = false;
    for (unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        if ( (ch == '-') && (x == 0) )
        {
            negative = true;
        }
        else if ( ch == '+' )
        {
            //  do nothing
        }
        else
        {
            result = result * 10 + (ch - '0');
        }
    }

    if ( negative )
        result = 0 - negative;
    return result;
}


//  getValueAsSigned64()
//
//  Interprets the value of this object as an integer
long long int
JSONNumberValue::getValueAsSigned64() const
{
    long long result = 0;
    bool negative = false;
    for (unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        if ( (ch == '-') && (x == 0) )
        {
            negative = true;
        }
        else if ( ch == '+' )
        {
            //  do nothing
        }
        else
        {
            result = result * 10 + (ch - '0');
        }
    }

    if ( negative )
        result = 0 - negative;
    return result;
}


//  getValueAsString()
//
//  Interprets the value of this object as a string
std::string
JSONNumberValue::getValueAsString() const
{
    return m_Value;
}


//  getValueAsUnsigned32()
//
//  Interprets the value of this object as an integer
unsigned long
JSONNumberValue::getValueAsUnsigned32() const
{
    unsigned long result = 0;
    for ( unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        result = result * 10 + (ch - '0');
    }

    return result;
}


//  getValueAsUnsigned64()
//
//  Interprets the value of this object as an integer
unsigned long long
JSONNumberValue::getValueAsUnsigned64() const
{
    unsigned long long result = 0;
    for ( unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        result = result * 10 + (ch - '0');
    }

    return result;
}


//  isNull()
//
//  Indicates whether this object has no value
bool
JSONNumberValue::isNull() const
{
    return false;
}


//  isZero()
//
//  Interprets this object's value as an integer, and indicates whether that integer is zero
bool
JSONNumberValue::isZero() const
{
    if ( m_Value.size() == 0 )
        throw JSONConversionException( "Cannot convert empty string to integer" );
    for ( unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        if ( ((ch == '-') || (ch == '+')) && (x == 0) )
        {
            //  ignore this
        }
        else if ( ch != '0' )
        {
            return false;
        }
    }

    return true;
}

