//  Activity class implementation



#include    "execlib.h"



//  statics

#ifdef  _DEBUG  //TODO:DEBUG
class   Tracker
{
private:
    std::set<Activity*>         m_ActivityTable;

public:
    ~Tracker()
    {
        if ( m_ActivityTable.size() > 0 )
        {
            OutputDebugString( "ERROR:Activity Tracking Table is NOT empty\n" );
            for ( std::set<Activity*>::const_iterator ita = m_ActivityTable.begin();
                    ita != m_ActivityTable.end(); ++ita )
            {
                char msg[100];
                sprintf_s(msg, 100, "Thread name: %s\n", (*ita)->getThreadName().c_str());
                OutputDebugString( msg );
            }
        }
    }

    void appendActivity( Activity* const pAct )     { m_ActivityTable.insert( pAct ); }
    void removeActivity( Activity* const pAct )     { m_ActivityTable.erase( pAct ); }
};

static Tracker      TrackingTable;
#endif



//  constructor, destructors

Activity::Activity
(
    Exec* const         pExec,
    const std::string&  threadName,
    RunInfo* const      pRunInfo
)
:Worker( threadName ),
m_pExec( pExec ),
m_pRunInfo( pRunInfo )
{
#ifdef  _DEBUG
    TrackingTable.appendActivity( this );
#endif
}


Activity::~Activity()
{
#ifdef  _DEBUG
    TrackingTable.removeActivity( this );
#endif
}



//  public methods

//  dump()
//
//  For debugging.
//  If overloaded, the derived function is expected to call back here at some point.
//  We don't display anything about the owning RunInfo, as we expect this will be called from
//  RunInfo's dump() function, thus making the relationship obvious.
void
    Activity::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "Activity " << getWorkerName()
        << " - 0x" << std::hex << static_cast<void *>(this)
        << ( isActive() ? " ACTIVE" : "" )
        << ( isTerminated() ? " TERMINATED" : "" )
        << std::endl;
}


//  start()
//
//  Starts the activity
bool
Activity::start()
{
    return Worker::workerStart();
}


//  stop()
//
//  Forces this activity to stop prematurely
bool
Activity::stop
(
    const StopReason  reason,
    const bool        wait
)
{
    m_StopReason = reason;
    return Worker::workerStop( wait );
}



//  public statics

//  getStopReasonString()
std::string
Activity::getStopReasonString
(
    const StopReason        reason
)
{
    switch ( reason )
    {
    case STOP_NONE:         return "None";
    case STOP_SHUTDOWN:     return "Shutdown";
    case STOP_DEMAND_TERM:  return "Demand TERM";
    case STOP_DEMAND_X:     return "Demand @@X";
    }

    return "???";
}
