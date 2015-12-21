//  JSONStringValue.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Header file for JSONString object, modelling a JSON string value



#ifndef     JSONLIB_JSON_STRING_VALUE_H
#define     JSONLIB_JSON_STRING_VALUE_H



#include    "JSONAtomicValue.h"



class   JSONStringValue : public JSONAtomicValue
{
private:
    const std::string           m_Value;

    static bool                 equalsIgnoreCase( const std::string&    string1,
                                                  const std::string&    string2 );

public:
    JSONStringValue( const std::string& value )
        :JSONAtomicValue( VT_STRING ),
        m_Value(value)
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
