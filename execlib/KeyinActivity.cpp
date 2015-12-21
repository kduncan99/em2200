//  KeyinActivity class implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  statics



//  private, protected methods

//  displayGeneralError()
void
KeyinActivity::displayGeneralError() const
{
    std::string dispStr = m_KeyinId;
    dispStr += " KEYIN ERROR";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	displayInvalidOption()
//
//	useful protected funtion
void
KeyinActivity::displayInvalidOption() const
{
    std::string dispStr = "Invalid option for ";
    dispStr += m_KeyinId;
    dispStr += " KEYIN";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	displayInvalidParameter()
//
//	useful protected funtion
void
KeyinActivity::displayInvalidParameter() const
{
    std::string dispStr = "Invalid parameter for ";
    dispStr += m_KeyinId;
    dispStr += " KEYIN";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	displayNoParameters()
//
//	useful protected funtion
void
KeyinActivity::displayNoParameters() const
{
    std::string dispStr = "No parameters specified for  ";
    dispStr += m_KeyinId;
    dispStr += " keyin";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	displayOptionNotAllowed()
//
//	useful protected funtion
void
KeyinActivity::displayOptionNotAllowed() const
{
    std::string dispStr = "Option not allowed for ";
    dispStr += m_KeyinId;
    dispStr += " KEYIN";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	displayParametersNotAllowed()
//
//	useful protected funtion
void
KeyinActivity::displayParametersNotAllowed() const
{
    std::string dispStr = "Parameters not allowed for ";
    dispStr += m_KeyinId;
    dispStr += " KEYIN";
    m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
}


//	worker()
//
//	Executing starts here
void
    KeyinActivity::worker()
{
    if ( isAllowed() )
    {
        handler();
    }
    else
    {
        std::string dispStr = m_KeyinId;
        dispStr += " keyin is not allowed";
        m_pConsoleManager->postReadOnlyMessage( dispStr, m_Routing, m_pExec->getRunInfo() );
    }
}



// constructors / destructors

KeyinActivity::KeyinActivity
(
    Exec* const                     pExec,
	const std::string&		        keyinId,
	const std::string&		        option,
	const std::vector<SuperString>& parameters,
	const Word36&			        routing
)
    :IntrinsicActivity( pExec, "Keyin", pExec->getRunInfo() ),
	m_KeyinId( keyinId ),
	m_Option( option ),
	m_Parameters( parameters ),
	m_Routing( routing )
{
    m_pConsoleManager =
        dynamic_cast<ConsoleManager*>(pExec->getManager( Exec::MID_CONSOLE_MANAGER ));
}



//  public methods

void
KeyinActivity::dump
(
    std::ostream&           stream,
    const std::string&      prefix,
    const DUMPBITS          dumpBits
)
{
    stream << prefix << "KeyinActivity for " << m_KeyinId << " console keyin" << std::endl;
    stream << prefix << "  Option:         " << m_Option << std::endl;
    stream << prefix << "  Parameters:     ";
    bool first = true;
    for ( const SuperString& str : m_Parameters )
    {
        if ( !first )
            stream << ",";
        stream << str;
    }
    stream << std::endl;
    stream << prefix << "  Routing:        " << m_Routing.toOctal() << std::endl;

    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}



//  public static methods

//	createKeyin()
//
//	Creates a Keyin object of the correct type, given the console input
//
//	static
//
//	Parameters:
//      pExec:              pointer to Exec instance
//		Text:				console input text
//		Routing:			routing word indicating source console
//
//	Returns:
//		pointer to new object if successful, else NULL
KeyinActivity*
KeyinActivity::createKeyin
(
    Exec* const             pExec,
	const std::string&		text,
	const Word36&			routing
)
{
	// Parse input text into keyinString, optionString, and paramStrings
    SuperString cmdString;
    INDEX tx = 0;
    while ( (tx < text.size()) && (text[tx] != ASCII_SPACE) && (text[tx] != ',') )
        cmdString += text[tx++];

    std::string optionString;
    if ( (tx < text.size()) && (text[tx] == ',') )
    {
        ++tx;
        while ( (tx < text.size()) && (text[tx] != ASCII_SPACE) )
            optionString += text[tx++];
    }

    std::vector<SuperString> paramStrings;
    while ( tx < text.size() )
    {
        std::string tempStr;
        while ( (tx < text.size()) && (text[tx] == ASCII_SPACE) )
            ++tx;
        while ( (tx < text.size()) && (text[tx] != ASCII_SPACE) && (text[tx] != ',') )
            tempStr += text[tx++];
        if ( tx < text.size() )
            ++tx;
        if ( tempStr.size() > 0 )
            paramStrings.push_back( tempStr );
	}

    cmdString.foldToUpperCase();
    if ( cmdString.compare( "CJ" ) == 0 )
        return new CJKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "CS" ) == 0 )
        return new CSKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "DJ" ) == 0 )
        return new DJKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "DN" ) == 0 )
        return new DNKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "DU" ) == 0 )
        return new DUKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "FF" ) == 0 )
        return new FFKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "FS" ) == 0 )
        return new FSKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "MS" ) == 0 )
        return new MSKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "PREP" ) == 0 )
        return new PREPKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "RV" ) == 0 )
        return new RVKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "SJ" ) == 0 )
        return new SJKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "SS" ) == 0 )
        return new SSKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "SU" ) == 0 )
        return new SUKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "UP" ) == 0 )
        return new UPKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( cmdString.compare( "$!" ) == 0 )
        return new DollarBangKeyin( pExec, cmdString, optionString, paramStrings, routing );
    else if ( pExec->getConfiguration().getBoolValue( "EXKEYINS" ) )
    {
        if ( cmdString.compare( "PREP" ) == 0 )
            return new PREPKeyin( pExec, cmdString, optionString, paramStrings, routing );
    }

    return NULL;
}


