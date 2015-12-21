//	IoActivity.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Polls IoManager and handles unsolicited input therefrom



#include	"execlib.h"



//	private / protected methods

void
IoActivity::worker()
{
    while ( !isWorkerTerminating() )
    {
        if ( !m_pIoManager->pollPendingRequests() )
            miscSleep( 50 );
    }
}



// constructors / destructors

IoActivity::IoActivity
(
    Exec* const         pExec,
	IoManager* const	pIoManager
)
:IntrinsicActivity( pExec, "IoActivity", pExec->getRunInfo() ),
m_pIoManager( pIoManager )
{
}



//	public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
IoActivity::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    stream << prefix << "IoActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}


