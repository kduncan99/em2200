//  JSONValue.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Abstract base class for all JSON value objects



#ifndef     JSONLIB_JSON_VALUE_H
#define     JSONLIB_JSON_VALUE_H


class   JSONValue
{
public:
    //  helpful enumerations
    enum    ValueType
    {
        VT_ARRAY,
        VT_BOOLEAN,
        VT_NUMBER,
        VT_NULL,
        VT_OBJECT,
        VT_STRING,
    };

protected:
    const ValueType             m_Type;

    JSONValue( const ValueType type)
        :m_Type(type)
    {}

public:
    virtual ~JSONValue(){};

    //  Virtuals which must be implemented by derived classes
    virtual std::string         encode() const = 0;

    //  Virtuals which default to throwing a JSONTypeException
    virtual bool                containsKey( const std::string& key ) const;
    virtual unsigned int        getCount() const;
    virtual JSONValue*          getItem( const unsigned int index ) const;
    virtual JSONValue*          getItem( const std::string& key ) const;
    virtual bool                getValueAsBool() const;
    virtual long int            getValueAsSigned32() const;
    virtual long long int       getValueAsSigned64() const;
    virtual std::string         getValueAsString() const;
    virtual unsigned long       getValueAsUnsigned32() const;
    virtual unsigned long long  getValueAsUnsigned64() const;
    virtual bool                isEmpty() const;
    virtual bool                isNull() const;
    virtual bool                isZero() const;
};



#endif

