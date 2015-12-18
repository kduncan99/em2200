//  implementation of SuperString class



#include    "misclib.h"



//  SuperString constructors
SuperString::SuperString()
{
}


SuperString::SuperString
    (
    const std::string&  value
    )
    :std::string( value )
{
}


//  compareNoCase()
//
//  Case-insensitive compare function
int
SuperString::compareNoCase
(
    const std::string&  value
) const
{
    unsigned int tx = 0; // index into *this* string
    unsigned int vx = 0; // index into value string
    while ( tx < size() )
    {
        if ( vx >= value.size() )
            return 1;
        char tch = at( tx++ );
        char vch = value.at( vx++ );
        if ( islower( tch ) )
            tch = static_cast<char>(toupper( tch ));
        if ( islower ( vch ) )
            vch = static_cast<char>(toupper ( vch ));
        if ( tch < vch )
            return -1;
        if ( tch > vch )
            return 1;
    }

    if ( vx < value.size() )
        return -1;

    return 0;
}


//  convertWhiteSpace()
//
//  Convert all non-space white-space to spaces.
void
SuperString::convertWhiteSpace()
{
    for ( INDEX sx = 0; sx < size(); ++sx )
    {
        if ( (*this)[sx] < ASCII_SPACE )
            (*this)[sx] = ASCII_SPACE;
    }
}


//  foldToUpperCase()
//
//  converts all lowercase to uppercase
void
SuperString::foldToUpperCase()
{
    for ( INDEX sx = 0; sx < size(); ++sx )
    {
        if ( islower( (*this)[sx] ) )
            (*this)[sx] = static_cast<char>(toupper( (*this)[sx] ));
    }
}


//  isDecimalNumeric()
//
//  Tests whether the content of the string is composed exclusively of decimal integers.
//  An empty string fails the test.
bool
SuperString::isDecimalNumeric() const
{
    size_type sc = size();
    if ( sc == 0 )
        return false;
    for ( INDEX sx = 0; sx < sc; ++sx )
    {
        if ( !isdigit( (*this)[sx] ) )
            return false;
    }

    return true;
}


//  strip()
//
//  Strips characters from this string into the result string, up to the delimiter character.
//  The delimiter character is stripped from this string, but not added to the result string.
//  If no delimiter is found, the remainder of this string is stripped, and returned in the result.
std::string
SuperString::strip
(
    const char      delimiter
)
{
    std::string result;
    INDEX sx = 0;
    COUNT removeCount = 0;
    while ( sx < size() )
    {
        if ( (*this)[sx] == delimiter )
        {
            ++removeCount;
            break;
        }

        result += (*this)[sx];
        ++removeCount;
        ++sx;
    }

    if ( removeCount == size() )
        clear();
    else if ( removeCount > 0 )
        *this = substr( removeCount );

    return result;
}


//  toDecimal()
//
//  Converts the string to a corresponding 32-bit unsigned decimal value
UINT32
SuperString::toDecimal() const
{
    UINT32 result = 0;
    INDEX sx = 0;
    while (sx < size())
    {
        char ch = (*this)[sx++];
        if ( !isdigit(ch) )
            break;
        result = result * 10 + (ch - '0');
    }

    return result;
}


//  toDecimal64()
//
//  Converts the string to a corresponding 64-bit unsigned decimal value
UINT64
SuperString::toDecimal64() const
{
    UINT64 result = 0;
    INDEX sx = 0;
    while (sx < size())
    {
        char ch = (*this)[sx++];
        if ( !isdigit(ch) )
            break;
        result = result * 10 + (ch - '0');
    }

    return result;
}


//  trimLeadingSpaces()
//
//  removes leading spaces
void
SuperString::trimLeadingSpaces()
{
    INDEX sx = 0;
    while ( sx < size() )
    {
        if ( (*this)[sx] != ASCII_SPACE )
            break;
        ++sx;
    }

    if ( sx > 0 )
        *this = substr( sx );
}


//  trimTrailingSpaces()
//
//  removes trailing spaces
void
SuperString::trimTrailingSpaces()
{
    size_type sx = size();
    while ( sx > 0 )
    {
        if ( (*this)[sx - 1] != ASCII_SPACE )
            break;
        --sx;
    }

    if ( sx < size() )
        resize( sx );
}


//  toWide()
//
//  Converts to wide string
std::wstring
SuperString::toWide() const
{
    std::wstring wstr;
    for ( unsigned int x = 0; x < size(); ++x )
        wstr += (wchar_t) at(x);
    return wstr;
}


SuperString&
SuperString::operator=
(
    const std::wstring& source
)
{
    clear();
    for ( unsigned int x = 0; x < source.size(); ++x )
        (*this) += (char) source[x];
    return *this;
}


SuperString&
SuperString::operator=
(
    const SuperString&  source
)
{
    clear();
    append( source );
    return *this;
}


SuperString&
SuperString::operator=
(
    const char* const   pSource
)
{
    clear();
    append( pSource );
    return *this;
}


SuperString&
SuperString::operator=
(
    char*               pSource
)
{
    clear();
    append( pSource );
    return *this;
}


