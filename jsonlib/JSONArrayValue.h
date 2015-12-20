//  JSONArrayValue.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Describes a JSON array (of other JSON objects).



#ifndef     JSONLIB_JSON_ARRAY_VALUE_H
#define     JSONLIB_JSON_ARRAY_VALUE_H



#include    "JSONCompositeValue.h"



class   JSONArrayValue : public JSONCompositeValue
{
private:
    std::list<JSONValue*>       m_Values;

public:
    JSONArrayValue()
        :JSONCompositeValue( VT_ARRAY )
    {}

    ~JSONArrayValue();

    void                        append( JSONValue* const pValue );

    //  JSONCompositeValue interface
    std::string                 encode() const;
    unsigned int                getCount() const;
    JSONValue*                  getItem( const unsigned int index ) const;
    bool                        isEmpty() const;
};



#endif
