//  TransparentCSInterpreter implementation
//  Copyright (c) 2015 by Kurt Duncan
//
//  Interprets transparent control statements


#include    "execlib.h"



//  private methods

//  scanCharacter()
//
//  Scans ahead for a particular character
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanCharacter
(
    const char          ch
)
{
    if ( !atEnd() && (m_Statement[m_Index] == ch) )
    {
        ++m_Index;
        return TCSIST_SUCCESSFUL;
    }

    return TCSIST_NOT_FOUND;
}


//  scanMasterSpaces()
//
//  Ensures there are masterspaces in columns 1 and 2 of the statement.
//  If not, we return false, allowing the caller to decide what to do.
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanMasterSpaces()
{
    if ( (m_Statement.size() < 2) || (m_Statement[0] != '@') || (m_Statement[1] != '@') )
        return TCSIST_NOT_FOUND;
    m_Index += 2;

    return TCSIST_SUCCESSFUL;
}


//  scanRemainderCM()
//
//  Scans the remainder of the transparent command
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanRemainderCM()
{
    m_Command = TCMD_CM;

    //  Make sure we don't have a comma
    if ( scanCharacter( ',' ) == TCSIST_SUCCESSFUL )
        return TCSIST_SYNTAX_ERROR;

    skipWhiteSpace();

    while ( !atEnd() )
        m_Text += m_Statement[m_Index++];
    m_Text.trimTrailingSpaces();

    return TCSIST_SUCCESSFUL;
}


//  scanRemainderMSG()
//
//  Scans the remainder of the transparent command
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanRemainderMSG()
{
    m_Command = TCMD_MSG;

    //  Make sure we don't have a comma
    if ( scanCharacter( ',' ) == TCSIST_SUCCESSFUL )
        return TCSIST_SYNTAX_ERROR;

    skipWhiteSpace();

    while ( !atEnd() )
        m_Text += m_Statement[m_Index++];
    m_Text.trimTrailingSpaces();

    return TCSIST_SUCCESSFUL;
}


//  scanRemainderTERM()
//
//  Scans the remainder of the transparent command
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanRemainderTERM()
{
    m_Command = TCMD_TERM;

    //  Make sure we don't have a comma
    if ( scanCharacter( ',' ) == TCSIST_SUCCESSFUL )
        return TCSIST_SYNTAX_ERROR;

    skipWhiteSpace();
    if ( !atEnd() )
        return TCSIST_SYNTAX_ERROR;

    return TCSIST_SUCCESSFUL;
}


//  scanRemainderX()
//
//  Scans the remainder of the transparent command
TransparentCSInterpreter::Status
TransparentCSInterpreter::scanRemainderX()
{
    m_Command = TCMD_X;

    //  Make sure we don't have a comma
    if ( scanCharacter( ',' ) == TCSIST_SUCCESSFUL )
        return TCSIST_SYNTAX_ERROR;

    skipWhiteSpace();

    //  Read single-character commands; C, I, O, R, and T are allowed
    //  We store these commands as options, so don't be confused.
    bool comma = false;
    while ( !atEnd() && !comma )
    {
        char ch = m_Statement[m_Index++];
        if ( islower( ch ) )
            ch = toupper( ch );
        switch ( ch )
        {
        case ',':
            comma = true;
            break;

        case 'C':
            m_Options |= OPTB_C;
            break;

        case 'I':
            m_Options |= OPTB_I;
            break;

        case 'O':
            m_Options |= OPTB_O;
            break;

        case 'R':
            m_Options |= OPTB_R;
            break;

        case 'T':
            m_Options |= OPTB_T;
            break;

        default:
            return TCSIST_SYNTAX_ERROR;
        }
    }

    if ( m_Options == 0 )
        m_Options = OPTB_C | OPTB_I | OPTB_O | OPTB_T;

    //  Read text (possibly)
    if ( comma )
    {
        skipWhiteSpace();
        while ( !atEnd() && (m_Statement[m_Index] != ' ') )
            m_Text += m_Statement[m_Index++];

        //  Anything after the next set of spaces is ignored
    }

    return TCSIST_SUCCESSFUL;
}



//  constructors, destructors

//  Parameters:
//      pActivity:      Pointer to RSIActivity which is requesting this interpretation
//      statement:      Statement to be interpreted
TransparentCSInterpreter::TransparentCSInterpreter
(
    const std::string&      statement
)
:m_Command( TCMD_INIT ),
m_Index( 0 ),
m_Options( 0 ),
m_Statement( statement )
{
}



//  public methods

//  interpretStatement()
//
//  Interprets the object's control statement.  Does NOT execute anything.
TransparentCSInterpreter::Status
TransparentCSInterpreter::interpretStatement()
{
    //  Make sure we have two consecutive masterspaces
    Status status = scanMasterSpaces();
    if ( status != TCSIST_SUCCESSFUL )
        return TCSIST_NOT_FOUND;

    //  Get statement - we're looking *ONLY* for alphabetic characters, followed by a valid terminator.
    //  Given that requirement, we then look at the token, and check it for known values.
    //  If we don't find such a value, this is not a known control statement and we return NOT_FOUND.
    SuperString command;
    while ( !atEnd() && isalpha(m_Statement[m_Index]) )
        command += m_Statement[m_Index++];
    if ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if ( (ch != ASCII_SPACE) && (ch != ',') )
            return TCSIST_SYNTAX_ERROR;
    }

    //  If we have no command, this is an error.
    if ( command.size() == 0 )
        return TCSIST_SYNTAX_ERROR;

    //  See if we recognize the transparent command
    command.foldToUpperCase();
    if ( command.compare( "CM" ) == 0 )
        return scanRemainderCM();
    if ( command.compare( "MSG" ) == 0 )
        return scanRemainderMSG();
    if ( command.compare( "TERM" ) == 0 )
        return scanRemainderTERM();
    if ( command.compare( "X" ) == 0 )
        return scanRemainderX();

    return TCSIST_SYNTAX_ERROR;
}



//  Static publics

//  getStatusDisplayString()
//
//  Converts Status enum to a displayable string
std::string
TransparentCSInterpreter::getStatusString
(
    const Status            status
)
{
    switch ( status )
    {
    case TCSIST_SUCCESSFUL:                 return "Successful";
    case TCSIST_NOT_FOUND:                  return "Not Found";
    case TCSIST_SYNTAX_ERROR:               return "Syntax Error";
    }

    return "???";
}


//  extraction operator
//
//  for debugging use only
#ifdef _DEBUG
std::ostream&
operator<<
(
    std::ostream&                   stream,
    const TransparentCSInterpreter& csInterpreter
)
{
    stream << "Input='" << csInterpreter.getStatement() << "'"
        << " Index=" << csInterpreter.getIndex();

    const SuperString& text = csInterpreter.getText();
    const UINT32 options = csInterpreter.getOptions();

    switch ( csInterpreter.getCommand() )
    {
    case TransparentCSInterpreter::TCMD_CM:
        stream << "TCMD_CM"
            << " Text='" << text << "'"
            << std::endl;
        break;

    case TransparentCSInterpreter::TCMD_INIT:
        stream << "TCMD_INIT (initial mode)" << std::endl;
        break;

    case TransparentCSInterpreter::TCMD_MSG:
        stream << "TCMD_MSG"
            << " Text='" << text << "'"
            << std::endl;
        break;

    case TransparentCSInterpreter::TCMD_TERM:
        stream << "TCMD_TERM" << std::endl;
        break;

    case TransparentCSInterpreter::TCMD_X:
        stream << "TCMD_X"
            << " Cmds="
            << ((options == 0) ? "<None>" : "")
            << ((options & OPTB_C) ? "C" : "")
            << ((options & OPTB_I) ? "I" : "")
            << ((options & OPTB_O) ? "O" : "")
            << ((options & OPTB_R) ? "R" : "")
            << ((options & OPTB_T) ? "T" : "")
            << " Text='" << text << "'"
            << std::endl;
        break;
    }

    return stream;
}
#endif


