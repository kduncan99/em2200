//  Configuration
//
//  Loaded by emexec, and passed to Exec upon instantiation.
//  It is conceivable that the owner (emexec) can update this while it is held by Exec;
//  This means that Exec must protect itself from damage which could be caused by such
//  antics.  For example, there are some values (e.g., DCLUTS) which should *NOT* change
//  while Exec is running (said change requires an MFD initialization)...  Thus, Exec
//  should read this value at boot time, and hold it elsewhere until the next boot.
//  Other values might be changeable dynamically (perhaps the value indicating whether
//  a processor call starts a new page, or something like that)...



#ifndef     EXECLIB_CONFIGURATION_H
#define     EXECLIB_CONFIGURATION_H



class   Configuration
{
public:
    //  -----------------------------------------------------------------------
    //  Fixed key values
    //  -----------------------------------------------------------------------
    //???? need to make all the keys statics here, and use them in the code

    //  -----------------------------------------------------------------------
    //  Classes for representing basic configuration values
    //  -----------------------------------------------------------------------
    enum ValueType
    {
        VT_BOOL,
        VT_FLOAT,
        VT_INTEGER,
        VT_STRING,
        VT_VECTOR,
    };

    class Value
    {
    public:
        Value(){}
        virtual ~Value(){}
        virtual Value*      copy() const = 0;
        virtual ValueType   getType() const = 0;
    };

    class BoolValue : public Value
    {
    public:
        const bool              m_Value;
        BoolValue( const bool value ) : m_Value( value ){}
        inline Value*       copy() const    { return new BoolValue( m_Value ); }
        inline ValueType    getType() const { return VT_BOOL; }
    };

    class FloatValue : public Value
    {
    public:
        const float             m_Value;
        FloatValue( const float value ) : m_Value( value ){}
        inline Value*       copy() const    { return new FloatValue( m_Value ); }
        inline ValueType    getType() const { return VT_FLOAT; }
    };

    class IntegerValue : public Value
    {
    public:
        const UINT64            m_Value;
        IntegerValue( const UINT64 value ) : m_Value( value ){}
        inline Value*       copy() const    { return new IntegerValue( m_Value ); }
        inline ValueType    getType() const { return VT_INTEGER; }
    };

    class StringValue : public Value
    {
    public:
        const SuperString       m_Value;
        StringValue( const std::string& value ) : m_Value( value ){}
        inline Value*       copy() const    { return new StringValue( m_Value ); }
        inline ValueType    getType() const { return VT_STRING; }
    };

    //This is weird  commented out for now????
//    class VectorValue : public Value
//    {
//    public:
//        const std::vector<const Value* const>   m_Vector;
//        VectorValue( const std::vector<const Value* const>& vector ) : m_Vector( vector ){}
//        ~VectorValue()
//        {
//            for ( INDEX vx = 0; vx < m_Vector.size(); ++vx )
//                delete m_Vector[vx];
//        }
//        inline Value*       copy() const    { return new VectorValue( m_Vector ); }
//        inline ValueType    getType() const { return VT_VECTOR; }
//    };

private:
    //  -----------------------------------------------------------------------
    //  Private data structures and such
    //  -----------------------------------------------------------------------
    typedef std::map<SuperString, const Value*> DICTIONARY;
    typedef DICTIONARY::iterator                ITDICTIONARY;
    typedef DICTIONARY::const_iterator          CITDICTIONARY;

    DICTIONARY                  m_Dictionary;


    //  -----------------------------------------------------------------------
    //  Internal functions
    //  -----------------------------------------------------------------------
    bool                        establishValue( const std::string&  key,
                                                const Value* const  pValue );
    const Value*                getEntry( const SuperString& key ) const;

public:
    Configuration();
    ~Configuration();

    //  -----------------------------------------------------------------------
    //  Public functions for retrieving basic configuration values
    //  -----------------------------------------------------------------------
    bool                            getBoolValue( const std::string& key ) const;
    float                           getFloatValue( const std::string& key ) const;
    UINT64                          getIntegerValue( const std::string& key ) const;
    const SuperString&              getStringValue( const std::string& key ) const;
};



#endif
