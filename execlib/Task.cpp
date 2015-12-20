//  Task
//  Copyright (c) 2015 by Kurt Duncan
//
//  Some basic functionality for Task class



#include    "execlib.h"



//  constructors, destructors

Task::Task
    (
    const std::string&  runId,
    const std::string&  taskName
    )
    :m_RunId( runId ),
    m_TaskName( taskName )
{
}


Task::~Task()
{
    assert( !hasLiveActivity() );
    cleanActivities();
    assert( m_Activities.size() == 0 );
}



//  public methods

//  cleanActivities()
//
//  Deletes any Activity objects which are terminated
void
    Task::cleanActivities()
{
    lock();

    ITACTIVITIES itbase = m_Activities.begin();
    while ( itbase != m_Activities.end() )
    {
        if ( (*itbase)->isTerminated() )
        {
            ITACTIVITIES itnext = itbase;
            ++itnext;
            delete *itbase;
            m_Activities.erase( itbase );
            itbase = itnext;
        }
        else
            ++itbase;
    }

    unlock();
}


//  dump()
//
//  For debugging
void
    Task::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const DUMPBITS          dumpBits
    ) const
{
    lock();

    stream << prefix << "Task:" << m_TaskName << " for " << m_RunId << std::endl;
    stream << prefix << "  Activity count: " << m_Activities.size() << std::endl;
    for ( CITACTIVITIES ita = m_Activities.begin(); ita != m_Activities.end(); ++ita )
        (*ita)->dump( stream, prefix + "  ", dumpBits );

    unlock();
}


//  hasLiveActivity()
//
//  Returns true if any activity in the Task is not yet terminated
bool
    Task::hasLiveActivity() const
{
    bool result = false;
    lock();

    for ( CITACTIVITIES ita = m_Activities.begin(); ita != m_Activities.end(); ++ita )
    {
        if ( !(*ita)->isTerminated() )
        {
            result = true;
            break;
        }
    }

    unlock();
    return result;
}


//  killActivities()
//
//  Kills all activities - invoked during Exec shutdown only.  Does not wait.
void
Task::killActivities() const
{
    lock();
    for ( CITACTIVITIES ita = m_Activities.begin(); ita != m_Activities.end(); ++ita )
        (*ita)->stop( Activity::STOP_SHUTDOWN, false );
    unlock();
}

