//	PollActivity.cpp
//
//	Various generic polling which needs to occur



#include	"execlib.h"



static Word36 Routing;



//	private / protected methods

//  newDayActions()
//
//  Runs every morning very soon after midnight
//  Responsible for midnight time/day message.
void
PollActivity::newDayActions() const
{
    EXECTIME execTime = m_pExec->getExecTime();
    m_pExec->displayTime( execTime, true, Routing );
}


//  oneSecondActions()
//
//  Runs every second - calls Exec routine which processes the RunInfo list.
//  Among other things, this will do FIN processing, bring runs out of backlog, etc
void
PollActivity::oneSecondActions()
{
    m_pExec->pollUserRunInfoObjects( this );
}


//  sixMinuteActions()
//
//  Runs every 6 minutes.
//  Displays T/D message on console
void
PollActivity::sixMinuteActions() const
{
    EXECTIME execTime = m_pExec->getExecTime();
    m_pExec->displayTime( execTime, false, Routing );
}


//  sixSecondActions()
//
//  Runs every 6 seconds.
//  Updates the system message on the console.
void
PollActivity::sixSecondActions() const
{
    static COUNT iteration = 0;

    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    std::string msg1;
    std::string msg2;

    switch ( iteration )
    {
    case 0:
    case 1:
        msg1 = SSKeyin::getStatusLine1();
        msg2 = SSKeyin::getStatusLine2( m_pExec, pConsMgr );
        ++iteration;
        break;

    case 2:
    case 3:
        msg1 = SSKeyin::getStatusLine3( m_pExec );
        msg2 = SSKeyin::getStatusLine4();
        ++iteration;
        if ( iteration == 4 )
            iteration = 0;
        break;
    }

    pConsMgr->postSystemMessages( msg1, msg2 );
}


//  worker()
void
PollActivity::worker()
{
    while ( !isWorkerTerminating() )
    {
        //  Time to see whether anything needs to happen.
        //  If the current time indicates a day-of-week different than what's in m_PreviousMidnightDayOfWeek,
        //  it's time to do midnight processing.
        EXECTIME currentExecTime = m_pExec->getExecTime();
        SystemTime* pCurrentExecTimeComp = SystemTime::createFromMicroseconds( currentExecTime );
        if ( pCurrentExecTimeComp->getDayOfWeek() != m_PreviousMidnightDayOfWeek )
        {
            newDayActions();
            m_PreviousMidnightDayOfWeek = pCurrentExecTimeComp->getDayOfWeek();
            delete pCurrentExecTimeComp;
            pCurrentExecTimeComp = 0;
            continue;
        }
        delete pCurrentExecTimeComp;
        pCurrentExecTimeComp = 0;

        //  If the current time is 6 minutes past the previous 6-minute time-stamp,
        //  set state to do 6-minute stuff (which will subsequently trigger 6-second stuff).
        if ( (currentExecTime - m_Previous6Minute) >= (360 * SystemTime::MICROSECONDS_PER_SECOND) )
        {
            sixMinuteActions();
            m_Previous6Minute = currentExecTime;
            continue;
        }

        //  Similarly for 6-second stuff...
        if ( (currentExecTime - m_Previous6Second) >= (6 * SystemTime::MICROSECONDS_PER_SECOND) )
        {
            sixSecondActions();
            m_Previous6Second = currentExecTime;
            continue;
        }

        //  Finally for 1-second stuff...
        if ( (currentExecTime - m_Previous1Second ) >= (1 * SystemTime::MICROSECONDS_PER_SECOND) )
        {
            oneSecondActions();
            m_Previous1Second = currentExecTime;
            continue;
        }

        miscSleep( 200 );
    }
}



// constructors / destructors

PollActivity::PollActivity
(
    Exec* const         pExec
)
:IntrinsicActivity( pExec, "PollActivity", pExec->getRunInfo() )
{
    //  Establish prevMidnight (the time/date we last did midnight processing) as now;
    //  when midnight passes, checkTime() will trigger new day actions.
    // then begin iterating until it is time to quit.
    EXECTIME execTime = pExec->getExecTime();
    SystemTime* pSystemTime = SystemTime::createFromMicroseconds( execTime );
    m_PreviousMidnightDayOfWeek = pSystemTime->getDayOfWeek();
    delete pSystemTime;
    pSystemTime = 0;

    //  Similarly for previous 6 second action, and 6 minute action.
    m_Previous6Minute = execTime;
    m_Previous6Second = execTime;
    m_Previous1Second = execTime;
}



//	public methods

void
PollActivity::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    stream << prefix << "PollActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}

