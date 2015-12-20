//  RSISession.cpp
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"


//  constructors, destructors

#if 0 //????
RSISession::RSISession
(
    Exec* const         pExec,
    const COUNT         sessionNumber,
    const std::string&  sessionName,
    SmartConsole* const pScreen
)
:m_pExec( pExec ),
m_pScreen( pScreen ),
m_SessionName( sessionName ),
m_SessionNumber( sessionNumber )
{
    time ( &m_LastInputTime );
    m_BypassRunCard         = 0;
    m_pRunInfo              = 0;
    m_State                 = STATE_CONNECTED;
    m_TimeoutFlag           = false;
    m_TimeoutSecs           = 60;
    m_TimeoutWarning        = false;
    m_pTransparentActivity  = 0;
    m_UserIdErrorCount      = 0;

    pScreen->registerListener( this );
}
#endif


#if 0 //????
RSISession::~RSISession()
{
    if ( m_pTransparentActivity )
    {
        assert( m_pTransparentActivity->isTerminated() );
        delete m_pTransparentActivity;
    }

    if ( m_pScreen )
    {
        m_pScreen->unregisterListener( this );
        m_pScreen->close();
        delete m_pScreen;
    }
}
#endif



//  public methods

//  dump()
//
//  For debugging
void
RSISession::dump
(
    std::ostream&           stream,
    const std::string&      prefix,
    const DUMPBITS          dumpBits
)
{
    SystemTime* pSystemTime = SystemTime::createFromMicroseconds( m_LastInputSystemTime );
    std::string lastInputStr = pSystemTime->getTimeStamp();
    delete pSystemTime;
    pSystemTime = 0;

    stream << prefix << "Session #" << m_SessionNumber << "  " << m_SessionName << std::endl;
    stream << prefix << "  State:          " << getStateString( m_State ) << std::endl;
    stream << prefix << "  UserId:         " << m_UserId << std::endl;
    stream << prefix << "  UserId Errors:  " << m_UserIdErrorCount << std::endl;
    stream << prefix << "  LastInputTime:  " << lastInputStr << " (system time, not offset)" << std::endl;
    stream << prefix << "  Timeout (Secs): " << m_TimeoutSecs << std::endl;
    stream << prefix << "  Timeout Warn:   " << ( m_TimeoutWarning ? "Yes" : "No" ) << std::endl;
    stream << prefix << "  Timed Out:      " << ( m_TimeoutFlag ? "Yes" : "No" ) << std::endl;
    if ( m_pTransparentActivity )
    {
        stream << prefix << "  Transparent Activity:" << std::endl;
        m_pTransparentActivity->dump( stream, "      ", dumpBits );
    }
    if ( m_pRunInfo )
        stream << prefix << "  Actual RunId:   " << m_pRunInfo->getActualRunId() << std::endl;
}


//  isTerminating()
//
//  Determines if this Session is in a terminating state
bool
RSISession::isTerminating() const
{
    return ( m_State == STATE_TERMINATE
                || m_State == STATE_TERMINATE_DELETE
                || m_State == STATE_TERMINATE_WAIT_ACT
                || m_State == STATE_TERMINATE_WAIT_RUNINFO );
}


//  Listener interface
//
//  When called, it means the screen element has closed itself, and we should kill the session.
void
RSISession::listenerEventTriggered
(
    Event* const        pEvent
)
{
#if 0 //????
    m_pScreen->close();
    setState( STATE_TERMINATE );
#endif
}


//  registerTransparentActivity()
//
//  Creates a transparent RSI activity for the session
void
RSISession::registerTransparentActivity
(
    TransparentCSInterpreter* const     pInterpreter
)
{
    assert( m_pTransparentActivity == 0 );
    RSIManager* const pRsiMgr = dynamic_cast<RSIManager*>( m_pExec->getManager( Exec::MID_RSI_MANAGER ) );
    m_pTransparentActivity = new TransparentActivity( m_pExec,
                                                      pRsiMgr,
                                                      pInterpreter,
                                                      m_pRunInfo,
                                                      m_SessionNumber,
                                                      m_SessionName );
    m_pTransparentActivity->start();
}


//  reset()
//
//  Resets the RSISession for re-use upon Exec reboot
void
RSISession::reset()
{
#if 0 //????
    time( &m_LastInputTime );

    //  Exec restart will delete the RunInfo, so we just need to release the pointer
    m_pRunInfo = 0;

    m_pScreen->reset();
    m_State = STATE_CONNECTED;
    m_TimeoutFlag = false;
    m_TimeoutSecs = 60;
    m_TimeoutWarning = false;
    delete m_pTransparentActivity;
    m_pTransparentActivity = 0;
    m_UserIdErrorCount = 0;
#endif
}


//  sendClearScreen()
//
//  Sends the codes necessary to clear the output screen
void
RSISession::sendClearScreen() const
{
#if 0 //????
    m_pScreen->reset();//TODO:RSI temporary until we do real UTS output
#endif
}


//  sendOutput()
//
//  Simplest form of sending output to the RSI session.
//  No formatting, spacing, page ejects, nothing.
//  TODO:RSI For now, we assume @@RLU 24-ish, and truncate to 80 columns; that needs to be genericized, fleshed out, etc.
bool
RSISession::sendOutput
(
    const std::string&      text
) const
{
#if 0 //????
    std::string modText = text;
    if ( modText.size() > 80 )
        modText.resize( 80 );

    bool result = m_pScreen->deleteRow( 1 );
    if ( result )
        result = m_pScreen->writeRow( 24, COLOR_GREEN, COLOR_BLACK, modText );

    return result;
#else
    return false;
#endif
}


//  unregisterTransparentActivity()
//
//  Deletes the (hopefully terminated) transparent activity previously registered
void
RSISession::unregisterTransparentActivity()
{
    assert( m_pTransparentActivity && ( m_pTransparentActivity->isTerminated() ) );
    delete m_pTransparentActivity;
    m_pTransparentActivity = 0;
}



//  public statics

//  getSessionStateString()
//
//  For debugging
std::string
RSISession::getStateString
(
    const State         state
)
{
    switch ( state )
    {
    case STATE_ACTIVE:                      return "ACTIVE";
    case STATE_CONNECTED:                   return "CONNECTED";
    case STATE_INACTIVE:                    return "INACTIVE";
    case STATE_LOGIN_SOLICITED:             return "LOGIN SOLICITED";
    case STATE_LOGIN_ACCEPTED:              return "LOGIN ACCEPTED";
    case STATE_SOLICIT_LOGIN:               return "SOLICIT LOGIN";
    case STATE_TERMINATE:                   return "TERMINATE";
    case STATE_TERMINATE_DELETE:            return "TERMINATE DELETE";
    case STATE_TERMINATE_WAIT_ACT:          return "TERMINATE WAIT ACT";
    case STATE_TERMINATE_WAIT_RUNINFO:      return "TERMINATE WAIT RUNINFO";
    case STATE_WAITING_FOR_RUN_IMAGE:       return "WAIT RUN IMAGE";
    }

    return "???";
}

