//  JSONParser.cpp
//
//  Implementation of JSONParser class



#include    "jsonlib.h"



//  -------------------------------------------------------------------------------------------------------------------------------
//  local stuff not defined in the header file
//  -------------------------------------------------------------------------------------------------------------------------------

//  state machine stuff for parsing a number
#define     ALL_DIGITS          "0123456789"
#define     NON_ZERO_DIGITS     "123456789"
#define     TOKEN_TERMINATORS   " \n\r],}"

enum    ParseNumberState
{
    PNS_INITIAL,
    PNS_POST_LEADING_NEGATIVE,
    PNS_POST_LEADING_ZERO,
    PNS_POST_LEADING_DIGIT,
    PNS_POST_DECIMAL,
    PNS_POST_DECIMAL_DIGIT,
    PNS_POST_EXPONENT_SIGN,
    PNS_POST_EXPONENT_INDICATOR,
    PNS_POST_EXPONENT_DIGIT,
    PNS_DONE,                       //  terminal state
    PNS_NOT_FOUND,                  //  terminal state
    PNS_SYNTAX_ERROR,               //  terminal state
};

//  state transition instance
struct  ParseNumberTransition
{
    ParseNumberState    _initialState;
    const char*         _acceptedCharacters;    //  If null, this entry defines the default transition
    ParseNumberState    _nextState;
};

//  state transition table
static ParseNumberTransition    TransitionTable[] =
{
    { PNS_INITIAL,                  "-",                PNS_POST_LEADING_NEGATIVE },
    { PNS_INITIAL,                  "0",                PNS_POST_LEADING_ZERO },
    { PNS_INITIAL,                  NON_ZERO_DIGITS,    PNS_POST_LEADING_DIGIT },
    { PNS_INITIAL,                  0,                  PNS_NOT_FOUND },
    { PNS_POST_LEADING_NEGATIVE,    "0",                PNS_POST_LEADING_ZERO },
    { PNS_POST_LEADING_NEGATIVE,    NON_ZERO_DIGITS,    PNS_POST_LEADING_DIGIT },
    { PNS_POST_LEADING_NEGATIVE,    0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_LEADING_ZERO,        ".",                PNS_POST_DECIMAL },
    { PNS_POST_LEADING_ZERO,        "eE",               PNS_POST_EXPONENT_INDICATOR },
    { PNS_POST_LEADING_ZERO,        TOKEN_TERMINATORS,  PNS_DONE },
    { PNS_POST_LEADING_ZERO,        0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_LEADING_DIGIT,       NON_ZERO_DIGITS,    PNS_POST_LEADING_DIGIT },
    { PNS_POST_LEADING_DIGIT,       ".",                PNS_POST_DECIMAL },
    { PNS_POST_LEADING_DIGIT,       "eE",               PNS_POST_EXPONENT_INDICATOR },
    { PNS_POST_LEADING_DIGIT,       TOKEN_TERMINATORS,  PNS_DONE },
    { PNS_POST_LEADING_DIGIT,       0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_DECIMAL,             ALL_DIGITS,         PNS_POST_DECIMAL_DIGIT },
    { PNS_POST_DECIMAL,             TOKEN_TERMINATORS,  PNS_DONE },
    { PNS_POST_DECIMAL,             0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_DECIMAL_DIGIT,       ALL_DIGITS,         PNS_POST_DECIMAL_DIGIT },
    { PNS_POST_DECIMAL_DIGIT,       "eE",               PNS_POST_EXPONENT_INDICATOR },
    { PNS_POST_DECIMAL_DIGIT,       TOKEN_TERMINATORS,  PNS_DONE },
    { PNS_POST_DECIMAL_DIGIT,       0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_EXPONENT_INDICATOR,  "+-",               PNS_POST_EXPONENT_SIGN },
    { PNS_POST_EXPONENT_SIGN,       ALL_DIGITS,         PNS_POST_EXPONENT_DIGIT },
    { PNS_POST_EXPONENT_SIGN,       0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_EXPONENT_INDICATOR,  ALL_DIGITS,         PNS_POST_EXPONENT_DIGIT },
    { PNS_POST_EXPONENT_INDICATOR,  0,                  PNS_SYNTAX_ERROR },
    { PNS_POST_EXPONENT_DIGIT,      ALL_DIGITS,         PNS_POST_EXPONENT_DIGIT },
    { PNS_POST_EXPONENT_DIGIT,      TOKEN_TERMINATORS,  PNS_DONE },
    { PNS_POST_EXPONENT_DIGIT,      0,                  PNS_SYNTAX_ERROR },
};

static unsigned int TransitionTableSize = sizeof(TransitionTable) / sizeof(ParseNumberTransition);


//  findTransition()
//
//  Given an initial state and a trigger character, we find the appropriate ParseNumberTransition entry
//  which defines the next state.  If we don't find an entry, we return null.
static ParseNumberTransition*
findTransition
(
    const ParseNumberState  initialState,
    const char              trigger
)
{
    for ( unsigned int tx = 0; tx < TransitionTableSize; ++tx )
    {
        ParseNumberTransition* pThisTransition = &TransitionTable[tx];
        if ( pThisTransition->_initialState == initialState )
        {
            //  If the entry's accepted character array pointer is null, this is the default entry - use it.
            //  Otherwise, check the trigger to see if it in the accepted character array.
            if ( (pThisTransition->_acceptedCharacters == 0) || strchr( pThisTransition->_acceptedCharacters, trigger ) )
                return pThisTransition;
        }
    }

    return 0;
}


static bool
isTerminalState
(
    const ParseNumberState  state
)
{
    return (state == PNS_DONE) || (state == PNS_SYNTAX_ERROR) || (state == PNS_NOT_FOUND);
};



//  -------------------------------------------------------------------------------------------------------------------------------
//  private instance functions
//  -------------------------------------------------------------------------------------------------------------------------------

//  parseStringEscape()
//
//  Parses a string escape sequence, presuming the index is pointing at one.
//  If so, we return the sequence; otherwise, we return an empty string.
//  We throw a JSONStringEscapeException if we do find an escape sequence, but it isn't valid.
std::string
JSONParser::parseStringEscape()
{
    std::string result;
    if ( getRemainingCharacterCount() < 2 )
        return result;

    unsigned int initialIndex = m_Index;

    char nextCh = m_pText->at( m_Index );
    if ( nextCh != '\\' )
        return result;
    result += nextCh;
    m_Index += 1;

    nextCh = getNextChar();
    switch (nextCh)
    {
    case '\"':
    case '\\':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
        result += nextCh;
        return result;

    case 'u':
        if ( getRemainingCharacterCount() < 4 )
        {
            m_Index = initialIndex;
            throw new JSONStringEscapeException();
        }

        for ( unsigned int dx = 0; dx < 4; ++dx )
        {
            nextCh = getNextChar();
            if ( !isxdigit( nextCh ) )
            {
                m_Index = initialIndex;
                throw new JSONStringEscapeException();
            }

            result += nextCh;
        }
        return result;
        break;
    }

    m_Index = initialIndex;
    throw new JSONStringEscapeException();
}


//  parseToken()
//
//  Parses (or skips past) the indicated token, presuming the index is sitting at the beginning thereof.
//  If successful, the index will be placed at the next character following the token.
//  Returns false if we do not find the token
bool
JSONParser::parseToken
(
    const std::string&      token,
    const bool              caseSensitive
)
{
    if ( token.size() > getRemainingCharacterCount() )
        return false;

    unsigned int initialIndex = m_Index;
    bool match = true;
    unsigned int tokenx = 0;
    unsigned int tokenc = static_cast<unsigned int>(token.size());
    while ( match && (tokenx < tokenc) )
    {
        char chtoken = token[tokenx];
        char chtext = getNextChar();
        if ( caseSensitive )
        {
            if ( islower( chtoken ) )
                chtoken = toupper( chtoken );
            if ( islower( chtext ) )
                chtext = toupper( chtext );
        }

        if ( chtoken != chtext )
            match = false;

        ++tokenx;
    }

    if ( !match )
        m_Index = initialIndex;
    return match;
}


//  skipWhiteSpace()
//
//  Moves the index to the first non-whitespace character, or to the end of the text
inline void
JSONParser::skipWhiteSpace()
{
    while ( true )
    {
        char nextCh = m_pText->at( m_Index );
        if ( isWhiteSpace( nextCh ) )
            m_Index += 1;
        else
            break;
    }
}


//  -------------------------------------------------------------------------------------------------------------------------------
//  private JSON parsers
//  At entrance, we expect no whitespace (caller must skip this before invoking any of these)
//  At exit, whitespace may follow (caller must deal with this)
//  -------------------------------------------------------------------------------------------------------------------------------

//  parseArray()
//
//  Returnes a newly-allocated JSONArrayValue object if one is created; null if no array is found.
JSONValue*
JSONParser::parseArray()
{
    if ( atEnd() || (m_pText->at( m_Index ) != '[') )
        return 0;
    m_Index += 1;

    JSONArrayValue* pArray = new JSONArrayValue();
    bool allowValue = true;
    bool allowComma = true;
    bool allowEnd = true;
    bool foundEnd = false;
    while ( !atEnd() && !foundEnd )
    {
        skipWhiteSpace();

        //  Look for ending bracket
        if ( allowEnd && ( m_pText->at( m_Index ) == ']' ) )
        {
            foundEnd = true;
            m_Index += 1;
            continue;
        }

        //  Look for comma
        if ( allowComma && ( m_pText->at( m_Index ) == ',' ) )
        {
            allowComma = false;
            allowValue = true;
            allowEnd = false;
            m_Index += 1;
            continue;
        }

        //  Look for value
        if ( allowValue )
        {
            JSONValue* pValue = parseValue();
            if ( pValue != 0 )
            {
                pArray->append( pValue );
                allowComma = true;
                allowEnd = true;
                allowValue = false;
                continue;
            }
        }

        //  Something is wrong.
        throw new JSONArrayFormatException();
    }

    if ( !foundEnd )
        throw new JSONArrayFormatException();

    return pArray;
}


//  parseBoolean()
//
//  If we can parse a 'false' or 'true' token, we create a new JSONBooleanValue object.  Otherwise we return 0.
JSONValue*
JSONParser::parseBoolean()
{
    if ( parseToken( "false", true ) )
        return new JSONBooleanValue( false );
    else if ( parseToken( "true", true ) )
        return new JSONBooleanValue( true );
    else
        return 0;
}


//  parseNull()
//
//  If we can parse a 'null' token, we create a new JSONNullValue object.  Otherwise we return 0.
JSONValue*
JSONParser::parseNull()
{
    if ( !parseToken( "null", true ) )
        return 0;
    else
        return new JSONNullValue();
}


//  parseNumber()
//
//  Attempts to parse a number from the indexed location of the input string.
//  If successful, we return a new JSONNumberValue object.
//  If we don't find a number, we return 0.
//  If something is wrong part-way into the number, we throw an appropriate exception.
JSONValue*
JSONParser::parseNumber()
{
    //  Implemented as a deterministic finite acceptor in a state machine.
    unsigned int initialIndex = m_Index;
    std::string value;
    ParseNumberState pnState = PNS_INITIAL;
    while ( !isTerminalState( pnState ) )
    {
        char ch = static_cast<char>( m_pText->at( m_Index ) );
        if ( ch == 0x00 )
            ch = ' ';

        ParseNumberTransition* pTransition = findTransition( pnState, ch );
        if ( pTransition == 0 )
            pnState = PNS_SYNTAX_ERROR;
        else
            pnState = pTransition->_nextState;

        if ( !isTerminalState( pnState ) )
        {
            value += ch;
            m_Index += 1;
        }
    }

    if ( pnState == PNS_NOT_FOUND )
    {
        m_Index = initialIndex;
        return 0;
    }
    else if ( pnState == PNS_SYNTAX_ERROR )
    {
        m_Index = initialIndex;
        throw new JSONNumberFormatException();
    }
    else
        return new JSONNumberValue( value );
}


//  parseObject()
//
//  Parses a JSON object - i.e., a set of key-value pairs.
//  If we don't find one, we return 0.
//  If we find an error during parsing, we throw one of many different parsing exceptions.
JSONValue*
JSONParser::parseObject()
{
    unsigned int initialIndex = m_Index;
    if ( atEnd() || ( m_pText->at( m_Index ) != '{' ) )
        return 0;

    //  skip opening bracket now that we've found it
    m_Index += 1;

    JSONObjectValue* pObject = new JSONObjectValue();
    while ( true )
    {
        skipWhiteSpace();

        //  Need a quote-delimited string - if not found, we're done.
        JSONStringValue* pKey = dynamic_cast<JSONStringValue*>( parseString() );
        if ( pKey == 0 )
            break;

        //  We found a key, so we must have a colon
        skipWhiteSpace();
        if ( atEnd() || ( m_pText->at( m_Index ) != ':' ) )
        {
            delete pObject;
            delete pKey;
            m_Index = initialIndex;
            throw new JSONObjectFormatException();
        }

        //  We found a key and a colon - adjust the index to account for the colon
        ++m_Index;

        //  Now go look for a value of any type
        skipWhiteSpace();
        JSONValue* pValue = parseValue();
        if ( pValue == 0 )
        {
            delete pObject;
            delete pKey;
            m_Index = initialIndex;
            throw new JSONObjectFormatException();
        }

        pObject->store( pKey->getValueAsString(), pValue );
        delete pKey;

        //  key/value taken care of, do we have a comma?
        skipWhiteSpace();
        if ( atEnd() || ( m_pText->at( m_Index ) != ',' ) )
            break;
        m_Index += 1;
    }

    //  At this point, we MUST have an ending bracket.
    skipWhiteSpace();
    if ( atEnd() || ( m_pText->at( m_Index ) != '}' ) )
    {
        delete pObject;
        m_Index = initialIndex;
        throw new JSONObjectFormatException();
    }

    m_Index += 1;
    return pObject;
}


//  parseString()
//
//  Parses a string value if there is one.  If not we return 0.
JSONValue*
JSONParser::parseString()
{
    unsigned int initialIndex = m_Index;
    if ( atEnd() || ( m_pText->at( m_Index ) != '"' ) )
        return 0;
    m_Index += 1;

    std::string value;
    bool endDelimiter = false;
    while ( !atEnd() )
    {
        std::string escape = parseStringEscape();
        if ( escape.size() > 0 )
        {
            value += escape;
            continue;
        }

        char ch = getNextChar();
        if ( ch == '"' )
        {
            endDelimiter = true;
            break;
        }
        else if ( ch < ' ' )
        {
            m_Index = initialIndex;
            throw new JSONStringContentException();
        }
        else
            value += ch;
    }

    if ( !endDelimiter )
    {
        m_Index = initialIndex;
        throw new JSONStringUnterminatedException();
    }

    return new JSONStringValue( value );
}


//  parseValue()
//
//  Parses a JSONValue of the appropriate type.
JSONValue*
JSONParser::parseValue()
{
    skipWhiteSpace();
    JSONValue* pValue = parseString();
    if ( pValue )
        return pValue;

    pValue = parseNumber();
    if ( pValue )
        return pValue;

    pValue = parseObject();
    if ( pValue )
        return pValue;

    pValue = parseArray();
    if ( pValue )
        return pValue;

    pValue = parseBoolean();
    if ( pValue )
        return pValue;

    return parseNull();
}


//  -------------------------------------------------------------------------------------------------------------------------------
//  private static functions
//  -------------------------------------------------------------------------------------------------------------------------------

//  isWhiteSpace()
//
//  Checks whether the given character can be considered white space
inline bool
JSONParser::isWhiteSpace
(
    char     ch
)
{
    return ( ( ch == ' ' ) || ( ch == '\n' ) || ( ch == '\r' ) || ( ch == 0x08 ) );
}



//  -------------------------------------------------------------------------------------------------------------------------------
//  public instance functions
//  -------------------------------------------------------------------------------------------------------------------------------

//  parse()
//
//  Main entry point - parses a properly-formatted string into a (possibly nested) JSONValue
JSONValue*
JSONParser::parse
(
    const std::string&      text
)
{
    m_pText = &text;
    m_Index = 0;
    return parseValue();
}

