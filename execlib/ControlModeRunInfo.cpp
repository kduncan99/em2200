//  ControlModeRunInfo implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors

ControlModeRunInfo::ControlModeRunInfo
(
    Exec* const         pExec,
    const State         initialState,
    const std::string&  originalRunId,
    const std::string&  actualRunId,
    const std::string&  accountId,
    const std::string&  projectId,
    const std::string&  userId,
    const UINT32        options,
    const char          schedulingPriority,
    const char          processorDispatchingPriority
)
:UserRunInfo( pExec,
              initialState,
              originalRunId,
              actualRunId,
              accountId,
              projectId,
              userId,
              options,
              schedulingPriority,
              processorDispatchingPriority )
{
    m_pControlModeActivity = 0;
    m_DataIgnoredMsgFlag = false;
    m_SkipCount = 0;
}


ControlModeRunInfo::~ControlModeRunInfo()
{
    if ( m_pControlModeActivity )
        delete m_pControlModeActivity;
}



//  public methods

//  cleanActivites()
//
//  Called occasionally by the Exec, so we can get rid of zombies.
//  Exec MUST attach before calling here.
//
//  Returns:
//      true generally, false if there is no current task
void
ControlModeRunInfo::cleanActivities()
{
    RunInfo::cleanActivities();
    if ( m_pControlModeActivity )
    {
        if ( m_pControlModeActivity->isTerminated() )
        {
            delete m_pControlModeActivity;
            m_pControlModeActivity = 0;
        }
    }
}


//  dump()
//
//  For debugging
void
    ControlModeRunInfo::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    lock();

    UserRunInfo::dump( stream, prefix, dumpBits );
    stream << prefix << "  Data Ignored Msg Flag:  " << (m_DataIgnoredMsgFlag ? "Set" : "Clear") << std::endl;
    stream << prefix << "  Statement Image Stack:  " << (m_StatementImageStack.size() == 0 ? "<empty>" : "") << std::endl;
    if ( m_StatementImageStack.size() > 0 )
    {
        for ( LCITSTRING its = m_StatementImageStack.begin(); its != m_StatementImageStack.end(); ++its )
            stream << prefix << "    '" << *its << "'" << std::endl;
    }

    stream << prefix << "  Skip Count:             " << m_SkipCount << std::endl;
    stream << prefix << "  Skip Label:             " << m_SkipLabel << std::endl;

    //  Dump current control mode activity (if any)
    if ( m_pControlModeActivity )
    {
        stream << prefix << "  Control Mode Activity:" << std::endl;
        m_pControlModeActivity->dump( stream, prefix + "    ", dumpBits );
    }

    unlock();
}


//  hasLiveActivity()
//
//  Indicates whether this RunInfo has an activity that has not yet terminated
bool
    ControlModeRunInfo::hasLiveActivity() const
{
    bool result = false;
    lock();

    if ( RunInfo::hasLiveActivity() )
        result = true;
    else
    {
        if ( m_pControlModeActivity && ( !m_pControlModeActivity->isTerminated() ) )
            result = true;

    }

    unlock();
    return result;
}


//  killActivities()
//
//  Called by Exec during shut down.  Kill any still-active activities, and wait for them to die.
void
    ControlModeRunInfo::killActivities() const
{
    lock();
    RunInfo::killActivities();
    if ( m_pControlModeActivity && ( !m_pControlModeActivity->isTerminated() ) )
        m_pControlModeActivity->stop( Activity::STOP_SHUTDOWN, false );
    unlock();
}
