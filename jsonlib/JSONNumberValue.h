//  JSONNumberValue.h
//
//  Header file for JSONNumber object, modelling a JSON Number value



#ifndef     JSONLIB_JSON_NUMBER_VALUE_H
#define     JSONLIB_JSON_NUMBER_VALUE_H



#include    "JSONAtomicValue.h"



class   JSONNumberValue : public JSONAtomicValue
{
private:
    //  Represented internally as a UTF-8 string, since we need only simple ASCII characters
    const std::string           m_Value;

public:
    JSONNumberValue( const std::string& value )
        :JSONAtomicValue( VT_NUMBER ),
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
