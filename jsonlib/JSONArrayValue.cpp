//  JSONArrayValue.cpp
//
//  Implementation of JSONArrayValue class



#include    "jsonlib.h"



//  destructor
//
//  We need to delete all the owned JSONValue objects here
JSONArrayValue::~JSONArrayValue()
{
    for ( std::list<JSONValue*>::iterator it = m_Values.begin(); it != m_Values.end(); ++it )
        delete (*it);
}


//  append()
//
//  Appends a JSONValue to our internal list
void
JSONArrayValue::append
(
    JSONValue* const    pValue
)
{
    m_Values.push_back( pValue );
}


//  encode()
//
//  serializes the object into a displayable string, including appropriate newlines
std::string
JSONArrayValue::encode() const
{
    std::string result = "\n[ ";
    bool first = true;
    for ( std::list<JSONValue*>::const_iterator it = m_Values.begin(); it != m_Values.end(); ++it )
    {
        if ( !first )
            result += ", ";
        else
            first = false;

        result += (*it)->encode();
    }
    result += " ]";

    return result;
}


//  getCount()
//
//  Returns the size of the array
unsigned int
JSONArrayValue::getCount() const
{
    return static_cast<unsigned int>(m_Values.size());
}


//  getItem()
//
//  Returns the nth item in the array (zero-biased), or 0 if the index is out of range
JSONValue*
JSONArrayValue::getItem
(
    const unsigned int  index
) const
{
    std::list<JSONValue*>::const_iterator it = m_Values.begin();
    unsigned int ax = 0;
    while ( (ax < index) && ( it != m_Values.end() ) )
    {
        ++ax;
        ++it;
    }

    if ( it == m_Values.end() )
        return 0;
    return *it;
}


//  isEmpty()
//
//  Quick check to see whether the array is empty
bool
JSONArrayValue::isEmpty() const
{
    return m_Values.empty();
}

