//  JSONValue.cpp
//
//  Default methods used for cases where the superclass does not implement a particular function.
//  In all cases, we throw a JSONTypeException.



#include    "jsonlib.h"



bool
JSONValue::containsKey
(
    const std::string&  key
) const
{
    throw JSONTypeException( "containsKey() called on atomic value" );
}


unsigned int
JSONValue::getCount() const
{
    throw JSONTypeException( "getCount() called on atomic value" );
}


JSONValue*
JSONValue::getItem
(
    const unsigned int index
) const
{
    throw JSONTypeException( "getItem() called on atomic value" );
}


JSONValue*
JSONValue::getItem
(
    const std::string& key
) const
{
    throw JSONTypeException( "getItem() called on atomic value" );
}


bool
JSONValue::getValueAsBool() const
{
    throw JSONTypeException( "getValueAsBool() called on incorrect value type" );
}


long int
JSONValue::getValueAsSigned32() const
{
    throw JSONTypeException( "getValueAsSigned32() called on incorrect value type" );
}


long long int
JSONValue::getValueAsSigned64() const
{
    throw JSONTypeException( "getValueAsSigned64() called on incorrect value type" );
}


std::string
JSONValue::getValueAsString() const
{
    throw JSONTypeException( "getValueAsString() called on incorrect value type" );
}


unsigned long
JSONValue::getValueAsUnsigned32() const
{
    throw JSONTypeException( "getValueAsUnsigned32() called on incorrect value type" );
}


unsigned long long
JSONValue::getValueAsUnsigned64() const
{
    throw JSONTypeException( "getValueAsUnsigned64() called on incorrect value type" );
}


bool
JSONValue::isEmpty() const
{
    throw JSONTypeException( "isEmpty() called on atomic value" );
}


bool
JSONValue::isNull() const
{
    throw JSONTypeException( "isNull() called on incorrect value type" );
}


bool
JSONValue::isZero() const
{
    throw JSONTypeException( "isZero() called on incorrect value type" );
}

