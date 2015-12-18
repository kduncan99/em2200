//	ConsoleActivity.cpp
//
//	Polls ConsoleManager and handles unsolicited input therefrom



#include	"execlib.h"



//	private / protected methods

void
ConsoleActivity::worker()
{
    while ( !isWorkerTerminating() )
    {
        if ( !m_pConsoleManager->poll() )
            miscSleep( 250 );
    }
}



// constructors / destructors

ConsoleActivity::ConsoleActivity
(
Exec * const pExec,
ConsoleManager * const  pConsoleManager
)
:IntrinsicActivity( pExec, "ConsoleActivity", pExec->getRunInfo( ) ),
m_pConsoleManager( pConsoleManager )
{}



//	public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging

void
ConsoleActivity::dump
(
std::ostream&       stream,
const std::string&  prefix,
const DUMPBITS      dumpBits
)
{
    stream << prefix << "ConsoleActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}


