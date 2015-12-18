//  JSONAtomicValue.h
//
//  Base class for all JSON atomic values



#ifndef     JSONLIB_JSON_ATOMIC_VALUE_H
#define     JSONLIB_JSON_ATOMIC_VALUE_H



#include    "JSONValue.h"



class   JSONAtomicValue : public JSONValue
{
protected:
    JSONAtomicValue( const ValueType type )
        :JSONValue( type )
    {}
};



#endif

