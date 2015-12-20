//  JSONStringValue.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implements the JSONStringValue class



#include    "jsonlib.h"


//  encode()
//
//  Serializes the value of this object
std::string
JSONStringValue::encode() const
{
    std::string result;
    result = "\"" + m_Value + "\"";//???? need to escape special stuff
    return result;
}


//  compareIgnoreCase()
//
//  Compares two std::string objects, without respect to case
bool
JSONStringValue::equalsIgnoreCase
(
    const std::string&  string1,
    const std::string&  string2
)
{
    int sx = 0;
    int size1 = static_cast<unsigned int>(string1.size());
    int size2 = static_cast<unsigned int>(string2.size());

    while ((sx < size1) && (sx < size2))
    {
        char ch1 = string1[sx];
        if ( islower(ch1) )
            ch1 = toupper( ch1 );
        char ch2 = string2[sx];
        if ( islower(ch2) )
            ch2 = toupper( ch2 );
        if ( ch1 != ch2 )
            return false;
    }

    //  We got here with no miscompares... if the strings are equal in length, we have a match.  Otherwise, not.
    return (size1 == size2);
}


//  getValueAsBool()
//
//  Interprets the value of this object as a boolean
bool
JSONStringValue::getValueAsBool() const
{
    if ( equalsIgnoreCase( m_Value, "TRUE" ) )
        return true;
    else if ( equalsIgnoreCase( m_Value, "FALSE" ) )
        return false;
    else
        throw JSONConversionException( "Cannot convert string value to boolean" );
}


//  getValueAsSigned32()
//
//  Interprets the value of this object as an integer
long int
JSONStringValue::getValueAsSigned32() const
{
    return static_cast<long int>( getValueAsSigned64() );
}


//  getValueAsSigned64()
//
//  Interprets the value of this object as an integer
long long int
JSONStringValue::getValueAsSigned64() const
{
    if ( m_Value.size() == 0 )
        throw JSONConversionException( "Cannot convert empty string to integer" );

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
        else if ( !isdigit( ch ) )
        {
            throw JSONConversionException( "Cannot convert non-numeric string to integer" );
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
JSONStringValue::getValueAsString() const
{
    return m_Value;
}


//  getValueAsUnsigned32()
//
//  Interprets the value of this object as an integer
unsigned long
JSONStringValue::getValueAsUnsigned32() const
{
    return static_cast<unsigned long>( getValueAsUnsigned64() );
}


//  getValueAsUnsigned64()
//
//  Interprets the value of this object as an integer
unsigned long long
JSONStringValue::getValueAsUnsigned64() const
{
    if ( m_Value.size() == 0 )
        throw JSONConversionException( "Cannot convert empty string to integer" );

    unsigned long long result = 0;
    for ( unsigned int x = 0; x < m_Value.size(); ++x )
    {
        char ch = m_Value[x];
        if ( !isdigit( ch ) )
            throw JSONConversionException( "Cannot convert non-numeric string to integer" );
        result = result * 10 + (ch - '0');
    }

    return result;
}


//  isNull()
//
//  Indicates whether this object has no value
bool
JSONStringValue::isNull() const
{
    return false;
}


//  isZero()
//
//  Interprets this object's value as an integer, and indicates whether that integer is zero
bool
JSONStringValue::isZero() const
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
        else if ( !isdigit( ch ) )
        {
            throw JSONConversionException( "Cannot convert non-numeric string to integer" );
        }
        else if ( ch != '0' )
        {
            return false;
        }
    }

    return true;
}

