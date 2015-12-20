//  JSONBooleanValue.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Header file for JSON boolean value



#ifndef     JSONLIB_JSON_BOOLEAN_VALUE_H
#define     JSONLIB_JSON_BOOLEAN_VALUE_H



#include    "JSONAtomicValue.h"



class   JSONBooleanValue : public JSONAtomicValue
{
private:
    const bool                  m_Value;

public:
    JSONBooleanValue( const bool value )
        :JSONAtomicValue( VT_BOOLEAN ),
        m_Value( value )
    {}

    //  JSONAtomicValue interface
    std::string                 encode() const;
    bool                        getValueAsBool() const;
    long int                    getValueAsSigned32() const;
    long long int               getValueAsSigned64() const;
    std::string                 getValueAsString() const;
    unsigned long               getValueAsUnsigned32() const;
    unsigned long long          getValueAsUnsigned64() const;
    bool                        isNull() const;
    bool                        isZero() const;
};



#endif
