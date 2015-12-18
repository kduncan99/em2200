//	IntrinsicActivity.cpp
//
//  Implements a state-machine-driven engine, for derived classes to use.



#include    "execlib.h"



//  local statics



//	private / protected methods

//	worker()
//
//	Main working code
void
    IntrinsicActivity::worker()
{
    m_CurrentState = m_InitialState;
    while ( !isWorkerTerminating() )
    {
        //  Ending state?
        if ( m_CurrentState == m_TerminalState )
        {
            Worker::workerSetTermFlag();
            continue;
        }

        //  Find the StateEntry corresponding to the current state
        CITSTATEENTRIES itse = m_StateEntries.find( m_CurrentState );
        if ( itse == m_StateEntries.end() )
        {
            std::stringstream strm;
            strm << getWorkerName() << " intrinsic activity state machine failure - no entry for state " << m_CurrentState;
            SystemLog::write( strm.str() );
            m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
            Worker::workerSetTermFlag();
            break;
        }

        //  Set next state and next delay values - the handler might override them.
        //  Then call the handler under lock...
        m_NextState = itse->second.m_NextState;
        m_DelayMSec = itse->second.m_DelayMSec;
        lock();
        itse->second.m_Handler( this );
        unlock();

        //  Delay in small increments, if requested.
        COUNT32 delayRemaining = m_DelayMSec;
        while ( (delayRemaining > 0) && (!isWorkerTerminating()) )
        {
            COUNT32 delayTime = (delayRemaining > m_DelayIncrement) ? m_DelayIncrement : delayRemaining;
            workerWait( delayTime );
            delayRemaining -= delayTime;
        }

        //  Update current state and we're done with this loop.
        m_CurrentState = m_NextState;
    }
}



// constructors / destructors

IntrinsicActivity::IntrinsicActivity
(
    Exec* const             pExec,
    const std::string&      activityName,
    RunInfo* const          pRunInfo
)
:Activity( pExec, activityName, pRunInfo ),
m_CurrentState( m_InitialState ),
m_DelayMSec( m_DelayIncrement ),
m_NextState( m_TerminalState )
{
}


IntrinsicActivity::~IntrinsicActivity()
{
}



//	public methods

//  dump()
//
//  For debugging.
//  If overloaded, the derived function is expected to call back here at some point.
void
    IntrinsicActivity::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "IntrinsicActivity - CurrentState=" << m_CurrentState
        << " NextState=" << m_NextState << " Delay=" << m_DelayMSec << "ms" << std::endl;
    stream << prefix << "  State Entries:" << std::endl;
    for ( CITSTATEENTRIES itse = m_StateEntries.begin(); itse != m_StateEntries.end(); ++itse )
    {
        stream << prefix << "    State=" << std::setw( 3 ) << std::setfill( '0' ) << itse->first
            << " Next=" << std::setw( 3 ) << std::setfill( '0' ) << itse->second.m_NextState
            << " Delay=" << std::setw( 4 ) << std::setfill( '0' ) << itse->second.m_DelayMSec << "ms"
            << std::endl;
    }

    Activity::dump( stream, prefix + "  ", dumpBits );
}

