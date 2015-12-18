//	DUKeyin.cpp
//
//	Handles the DU keyin

//TODO:BATCH Need to change the format to "DU MP[, {runid | dumpBits}]"



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed (always allowed)
bool
DUKeyin::isAllowed() const
{
    return true;
}


//	handler()
//
//	Called from Keyin base class worker() function
void
DUKeyin::handler()
{
	if ( m_Option.compareNoCase( "MP" ) != 0 )
    {
        m_pConsoleManager->postReadOnlyMessage( "MP Option required for DU Keyin", m_Routing, m_pExec->getRunInfo() );
        return;
    }

    UINT32 dumpBits = DUMP_NORMAL;
    SuperString ss;
    switch ( m_Parameters.size() )
    {
    case 0:
        break;

    case 1:
        ss = m_Parameters[0];
        if ( ss.compareNoCase( "ALL" ) == 0 )
            dumpBits = DUMP_ALL;
        else
        {
            char* end = 0;
            dumpBits = strtoul( m_Parameters[0].c_str(), &end, 8 );
        }
        break;

    default:
        displayInvalidParameter();
        return;
    }

    std::string fileName = m_pExec->dump( dumpBits );
    std::string consMsg = "Dump Created - File Name:" + fileName;
	m_pConsoleManager->postReadOnlyMessage( consMsg, m_Routing, m_pExec->getRunInfo() );
}



//  constructors / destructors

DUKeyin::DUKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:KeyinActivity( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods
