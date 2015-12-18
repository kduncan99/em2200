//  DataHandler.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Inspired by SSG - reads and writes data formatted as SSG data, caching it in memory for access
//  by any interested caller.  ???? Obsolete?



#ifndef     EM2200_DATA_HANDLER_H
#define     EM2200_DATA_HANDLER_H



#include    "Lockable.h"



class   DataHandler : public Lockable
{
private:
    //  Data is stored as strings in label/line_number/field_number/subfield_number context.
    //  A subfield is basically just a string
    class   DataSubfield : public std::string{};

    //  A field is an ordered, sparse vector of subfields - we implement it with a map keyed by subfield number (1-biased)
    class   DataField : public std::map<INDEX, DataSubfield*>
    {
    public:
        ~DataField();
    };

    //  A line is an ordered non-sparse list of fields - we implement it with a map keyed by field number (1-biased)
    class   DataLine : public std::map<INDEX, DataField*>
    {
    public:
        ~DataLine();
    };

    //  A set is an ordered non-sparse list of lines - we implement it with a map keyed by line number (1-biased)
    class   DataSet : public std::map<INDEX, DataLine*>
    {
    public:
        ~DataSet();
    };

    //  The entire dictionary, keyed by label
    class   DataMap : public std::map<std::string, DataSet*>
    {
    public:
        ~DataMap();
    };

    DataMap             m_DataMap;
    bool                m_NeedsPersisted;

    bool                processInputLine( const std::string& text );

public:
    DataHandler()
        :m_NeedsPersisted( false )
    {}

    ~DataHandler()
    {
        assert( !m_NeedsPersisted );
    }

    void                clear();
    COUNT               get( const std::string& label ) const;
    COUNT               get( const std::string& label,
                             const INDEX        lineNumber ) const;
    COUNT               get( const std::string& label,
                             const INDEX        lineNumber,
                             const INDEX        fieldNumber ) const;
    const std::string*  get( const std::string& label,
                             const INDEX        lineNumber,
                             const INDEX        fieldNumber,
                             const INDEX        subfieldNumber ) const;
    bool                load( const std::string& fileName );
    bool                remove( const std::string&  label );
    bool                remove( const std::string&  label,
                                const INDEX         lineNumber );
    bool                remove( const std::string&  label,
                                const INDEX         lineNumber,
                                const INDEX         fieldNumber );
    bool                remove( const std::string&  label,
                                const INDEX         lineNumber,
                                const INDEX         fieldNumber,
                                const INDEX         subfieldNumber );
    void                save( const std::string& fileName );
    bool                set( const std::string& label,
                             const INDEX        lineNumber,
                             const INDEX        fieldNumber,
                             const INDEX        subfieldNumber,
                             const std::string& value );
};



#endif
