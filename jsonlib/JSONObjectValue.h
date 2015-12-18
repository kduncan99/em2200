//  JSONObjectValue.h
//
//  Models a JSON Object - that is, a record of key-value data; also described as a dictionary.



#ifndef     JSONLIB_JSON_OBJECT_VALUE_H
#define     JSONLIB_JSON_OBJECT_VALUE_H



#include    "JSONCompositeValue.h"



class   JSONObjectValue : public JSONCompositeValue
{
private:
    typedef std::map<std::string, JSONValue*>   CONTAINER;
    typedef CONTAINER::iterator                 ITCONTAINER;
    typedef CONTAINER::const_iterator           CITCONTAINER;

    CONTAINER                   m_Values;

public:
    JSONObjectValue()
        :JSONCompositeValue( VT_OBJECT )
    {}

    ~JSONObjectValue();

    void                        store( const std::string&   key,
                                       JSONValue* const     pValue );

    //  JSONCompositeValue interface
    bool                        containsKey( const std::string& key ) const;
    std::string                 encode() const;
    unsigned int                getCount() const;
    JSONValue*                  getItem( const std::string& key ) const;
    bool                        isEmpty() const;
};



#endif
