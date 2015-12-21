//  TransparentCSInterpreter
//  Copyright (c) 2015 by Kurt Duncan
//
//  Transparent control statement interpreter and executor


#ifndef     EXECLIB_TRANSPARENT_CS_INTERPRETER_H
#define     EXECLIB_TRANSPARENT_CS_INTERPRETER_H



class   TransparentCSInterpreter
{
public:
    enum Command
    {
        TCMD_INIT,              // Initial value

        //  Transparent ECL Statements
        TCMD_ASG,
        TCMD_BRKPT,
        TCMD_CAT,
        TCMD_FREE,
        TCMD_HDG,
        TCMD_LOG,
        TCMD_MODE,
        TCMD_MSG,
        TCMD_PASSWD,
        TCMD_QUAL,
        TCMD_START,
        TCMD_SYM,
        TCMD_USE,

        //  Transparent Symbiont Control Statements
        TCMD_CM,
        TCMD_CONS,
        TCMD_CONT,
        TCMD_CQUE,
        TCMD_DCT,
        TCMD_END,
        TCMD_ESC,
        TCMD_FUL,
        TCMD_HOLD,
        TCMD_INQ,
        TCMD_INS,
        TCMD_NOPR,
        TCMD_PMOD,
        TCMD_POC,
        TCMD_PRNT,
        TCMD_RLD,
        TCMD_RLU,
        TCMD_RQUE,
        TCMD_SEND,
        TCMD_SKIP,
        TCMD_TERM,
        TCMD_TM,
        TCMD_TOUT,
        TCMD_TTY,
        TCMD_X
    };

    enum Status
    {
        TCSIST_SUCCESSFUL,                  //  Valid statement which succeeded
        TCSIST_NOT_FOUND,                   //  Input does not begin with '@@'
        TCSIST_SYNTAX_ERROR,                //  General syntax error, or the statement was not recognized
    };

private:
    Command                         m_Command;
    INDEX                           m_Index;
    UINT32                          m_Options;
    const SuperString               m_Statement;
    SuperString                     m_Text;                 //  for @@CM, @@MSG

    Status              scanCharacter( const char ch );
    Status              scanMasterSpaces();
    Status              scanRemainderCM();
    Status              scanRemainderMSG();
    Status              scanRemainderTERM();
    Status              scanRemainderX();

    inline bool atEnd() const
    {
        return m_Index >= m_Statement.size();
    }

    inline COUNT skipWhiteSpace()
    {
        COUNT count = 0;
        while ( (m_Index < m_Statement.size()) && (m_Statement[m_Index] == ASCII_SPACE) )
        {
            ++m_Index;
            ++count;
        }
        return count;
    }

public:
    //  Caller must have filtered out comments, and properly concatenated continuation images
    //  before constructing the object.
    TransparentCSInterpreter( const std::string& statement );

    Status                      interpretStatement();

    inline Command              getCommand() const          { return m_Command; }
    inline char                 getCurrentCharacter() const { return m_Statement[m_Index]; }
    inline INDEX                getIndex() const            { return m_Index; }
    inline UINT32               getOptions() const          { return m_Options; }
    inline const SuperString&   getStatement() const        { return m_Statement; }
    inline const SuperString&   getText() const             { return m_Text; }

    static std::string  getStatusString( const Status status );
};


#ifdef  _DEBUG
std::ostream&       operator<< ( std::ostream& stream, const TransparentCSInterpreter& TransparentCSInterpreter );
#endif



#endif
