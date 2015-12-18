//  JSONParser.h
//
//  Describes the parser object which parses JSONValue objects out of a properly encoded string



#ifndef     JSONLIB_JSON_PARSER_H
#define     JSONLIB_JSON_PARSER_H



#include    "JSONValue.h"



class   JSONParser
{
private:
    unsigned int            m_Index;
    const std::string*      m_pText;

    std::string             parseStringEscape();
    bool                    parseToken( const std::string&  token,
                                        const bool          caseSensitive );
    void                    skipWhiteSpace();

    JSONValue*              parseArray();
    JSONValue*              parseBoolean();
    JSONValue*              parseNull();
    JSONValue*              parseNumber();
    JSONValue*              parseObject();
    JSONValue*              parseString();
    JSONValue*              parseValue();

    inline unsigned int     atEnd() const                       { return m_Index >= m_pText->size(); }
    inline char             getNextChar()                       { return m_pText->at( m_Index++ ); }
    inline unsigned int     getRemainingCharacterCount() const  { return static_cast<unsigned int>(m_pText->size() - m_Index); }

    static bool             isWhiteSpace( const char ch );

public:
    JSONValue*              parse( const std::string& text );
};



#endif
