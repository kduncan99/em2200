//  JSONCompositeValue.h
//
//  Base class for all composite values... i.e., objects and arrays



#ifndef     JSONLIB_JSON_COMPOSITE_VALUE_H
#define     JSONLIB_JSON_COMPOSITE_VALUE_H



#include    "JSONValue.h"



class   JSONCompositeValue : public JSONValue
{
protected:
    JSONCompositeValue( const ValueType type )
        :JSONValue( type )
    {}
};



#endif