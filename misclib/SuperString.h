//  SuperString
//
//  std::string superclass which does one or two additionally-helpful things



#ifndef     EM2200_SUPERSTRING_H
#define     EM2200_SUPERSTRING_H



class   SuperString : public std::string
{
public:
    SuperString();
    SuperString( const std::string& value );

    int             compareNoCase( const std::string& value ) const;
    void            convertWhiteSpace();
    void            foldToUpperCase();
    bool            isDecimalNumeric() const;
    void            trimLeadingSpaces();
    void            trimTrailingSpaces();
    std::string     strip( const char delimiter = ASCII_SPACE );
    UINT32          toDecimal() const;
    UINT64          toDecimal64() const;
    std::wstring    toWide() const;

    SuperString&    operator=( const std::wstring& source );
    SuperString&    operator=( const SuperString& source );
    SuperString&    operator=( const char* const pSource );
    SuperString&    operator=( char* pSource );
};



#endif