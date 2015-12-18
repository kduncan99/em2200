//  RSIActivity.cpp



#include    "execlib.h"



//  private, protected methods

//  worker()
void
RSIActivity::worker()
{
    m_pRSIManager->setActivity( this );

    while ( !isWorkerTerminating() )
    {
        if ( !m_pRSIManager->poll() )
            miscSleep( 250 );
    }

    m_pRSIManager->setActivity( 0 );
}



//  constructors / destructors

RSIActivity::RSIActivity
(
    Exec* const         pExec,
    RSIManager* const   pRSIManager
)
:IntrinsicActivity( pExec, "RSIActivity", pExec->getRunInfo() ),
m_pRSIManager( pRSIManager )
{
}



//  public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
RSIActivity::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    stream << prefix << "RSIActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}

