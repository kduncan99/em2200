//  JSONObjectValue.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implementation of JSONObjectValue class



#include    "jsonlib.h"



//  destructor
//
//  Deallocate all JSONValue objects we own
JSONObjectValue::~JSONObjectValue()
{
    for ( ITCONTAINER it = m_Values.begin(); it != m_Values.end(); ++it )
        delete it->second;
}


//  encode()
//
//  Serializes the value of this object
std::string
JSONObjectValue::encode() const
{
    std::string result = "\n{";
    bool first = true;
    for ( CITCONTAINER it = m_Values.begin(); it != m_Values.end(); ++it )
    {
        if ( !first )
            result += ", ";
        else
            first = false;

        result += "\n\"" + it->first + "\" : " + it->second->encode();
    }
    result += "}";

    return result;
}


//  store()
//
//  Adds a value to the map
void
JSONObjectValue::store
(
    const std::string&      key,
    JSONValue* const        pValue
)
{
    CITCONTAINER it = m_Values.find( key );
    if ( it != m_Values.end() )
        delete it->second;
    m_Values[key] = pValue;
}


//  containsKey()
//
//  Indicates whether the given key can be found in our map
bool
JSONObjectValue::containsKey
(
    const std::string&  key
) const
{
    CITCONTAINER it = m_Values.find( key );
    return ( it != m_Values.end() );
}


//  getCount()
//
//  Retrieves the number of items in this object
unsigned int
JSONObjectValue::getCount() const
{
    return static_cast<unsigned int>(m_Values.size());
}


//  getItem()
//
//  Retrieves the item corresponding to the given key if found, else 0
JSONValue*
JSONObjectValue::getItem
(
    const std::string&  key
) const
{
    CITCONTAINER it = m_Values.find( key );
    if ( it == m_Values.end() )
        return 0;
    return it->second;
}


//  isEmpty()
//
//  Quick check to see whether the object is currently empty
bool
JSONObjectValue::isEmpty() const
{
    return m_Values.empty();
}

