//	RSIManager.cpp
//
//	Implementation of RSIManager class - debug mode only


/*TODO:DEBUG for reference
ST SYS$*RUN$.CONSOL,,,A/UID,FLIT
0-ENTER PASSWD/C.L. FOR : SYS$*RUN$.CONSOL
0 SECURI
CONSOL START
RSI CONSOL CONSOL - ACTV
0-CONSOL*DEMAND
CONSOL*ENTER USERID/PASSWORD CLEARANCE LEVEL
0 UID/SECURI
0-CONSOL*DEMAND
CONSOL**DESTROY USERID/PASSWORD CLEARANCE LEVEL*
CONSOL*UNISYS 1100 OPERATING SYSTEM LEV. 40R1.00*00
CONSOL*(RSI)*
0 @RUN RUN000,A/UID,FLIT
RUN000/CONSOL START
0-CONSOL*DEMAND
CONSOL*DATE: 022487 TIME: 000055
0 @ASG,T FILE.
0-CONSOL*DEMAND
CONSOL*I:002333 ASG COMPLETE.
0 @FIN
0-CONSOL*DEMAND
RUN000 FIN
CONSOL* RUNID: RUN000 ACCT: A PROJECT:
CONSOL*FLIT
CONSOL* RUN000 FIN
CONSOL* TIME: TOTAL: 00:00:00.000 CBSUPS: 000
CONSOL*000000
CONSOL* CPU: 00:00:00.000 I/O: 00:00:
CONSOL*00.000
CONSOL* CC/ER: 00:00:00.000 WAIT: 00:00:
CONSOL*00.000
CONSOL* SUAS USED: 0.00 SUAS REMAINING:
CONSOL* 0000.00
CONSOL* IMAGES READ: 0 PAGES: 0
CONSOL* START: 00:00:55 FEB 24,1993 FIN: 00:01:
CONSOL*07 FEB 24,1993
CONSOL**TERMINAL INACTIVE*
0 @@TERM
RSI CONSOL - INACTIVE
CONSOL*END OF RUN
CONSOL FIN
*/

/*TODO:DEBUG for reference
After you complete the sign-on process, you may receive one or more �messages of the
day� on your terminal; this is one method the site administrator uses for contacting all
the users of your system to inform you of special developments concerning your
system, or other appropriate information.
Following the messages of the day, the system informs you of your current session
number and the date and time of your last sign-on to the system.
Your current session number is the total number of valid sign-on attempts to the system
for your user-id. Initially 0, this value may be reset to 0 after you have made 999 valid
sign-on attempts.
*/

/*TODO:DEBUG for reference
>ENTER USERID/PASSWORD CLEARANCE LEVEL
>UID/SECURI
><clear screen>
>*DESTROY USERID/PASSWORD CLEARANCE LEVEL*
>UNISYS 1100 OPERATING SYSTEM LEV. 40R1.00*00 (RSI)*
>@RUN RUN000,A/UID,FLIT
                                                                    RUN000/CONSOL START
>DATE: 022487 TIME: 000055
>@ASG,T FILE.
>I:002333 ASG COMPLETE.
>@FIN
                                                                    RUN000 FIN
> RUNID: RUN000 ACCT: A PROJECT: FLIT
> RUN000 FIN
> TIME: TOTAL: 00:00:00.000 CBSUPS: 000000000
> CPU: 00:00:00.000 I/O: 00:00:00.000
> CC/ER: 00:00:00.000 WAIT: 00:00:00.000
> SUAS USED: 0.00 SUAS REMAINING: 0000.00
> IMAGES READ: 0 PAGES: 0
> START: 00:00:55 FEB 24,1993 FIN: 00:01:07 FEB 24,1993
>*TERMINAL INACTIVE*
>@@TERM
                                                                    RSI CONSOL - INACTIVE
>END OF RUN
                                                                    CONSOL FIN

*/

/*TODO:DEBUG for reference, to be removed later
DEMAND TERMINAL MESSAGES
    ID NOT ACCEPTED
        A user-id or password entered by the user is illegal.
        (where is this used?)
    -@@COMPLETE
    @@CONS WARNING - THE KEYIN TEXT WAS TRUNCATED. VERIFY THE RESULT
        You entered, through @@CONS, a keyin that had too many characters. The limit,
        not including the keyin prefix or read-and-reply number, is 66 characters for
        unsolicited keyins or 68 for replies to read-and-reply messages. The Exec
        truncated and executed the keyin. Check to make sure the keyin did what you
        intended.
    *DEV. NOT CONFIGD FOR OUTPUT FILES*
        An @@SEND was received from a demand terminal that is not configured for
        receiving batch output files.
    *END ERROR*
        You specified an improper mode on an @@END statement
    -@@ERROR (error codes)
    -@@ERROR - ILLEGAL TYPE
    -@@ERROR - SYNTAX
        These three messages indicate that an error was encountered while processing a
        transparent statement. The error codes are the same error codes returned on a
        dynamic request of the control statement.
    *EXECUTION TERMINATED *
        The execution of the demand run has been terminated by the demand symbiont
        @@X T statement.
    *HOLD ERR - SYNTAX*
        Syntax error on @@HOLD.
    *MSG QUEUE FULL*
        The addressee on an @@TM statement already has seven messages queued,
        the maximum allowed.
    @@PASSWD ERROR: INVALID USER-ID/PASSWORD
        The password you entered as your current password is not legal.
    *RUN-ID NOT ACTIVE*
        The run-id on an @@TM is not currently active.
    *SITE-ID NOT ACTIVE*
        The site-id specified on an @@TM statement is not currently active.
    *TERMINAL INACTIVE - OUTPUT FILE AVAILABLE*
        The terminal is inactive, and a batch output file is available. An @@SEND causes the
        file to be transmitted.
    *TIMEOUT DISABLED*
        The timeout mechanism is disabled because of an @@TOUT D statement.
        The terminal will not time out.
    *TIMEOUT PERIOD IS t MINUTES*
        The current terminal timeout period in minutes is indicated by t. When the timeout
        value indicated on the @@TOUT is greater than the timeout length allowed for the
        user-id, the new timeout period is the maximum value allowed for the user-id, as
        indicated by t.
    TM KEY ERROR
        Syntax error on @@TM.
    *USER-ID NOT ACTIVE*
        The user-id on an @@TM is not currently active.
    *WAIT-LAST INPUT IGNORED
        This applies only to demand terminals. The last input statement was rejected,
        probably because another statement was still being processed.
    *WARNING - CONSOLE OUTPUT LOST*
        Some output specified on an @@CONS statement may be lost.
    @@X ERROR - TOO MANY DATA CHARACTERS
        The data field cannot be more than 6 characters.
    *MORE INFO?* (A, X, R, J, E, B)
        This message is displayed when a demand program terminates after encountering
        an error or aborts after an error identification line has been printed. This facility lets
        the demand user obtain more information about the error. If no additional
        information is desired, you can enter another control statement. The responses
        available to this message are any combination of the following letters:
        A Printout of the A-registers
        X Printout of the X-registers
        R Printout of the R-registers
        J Jump history stack saved at the time of the error
        E Message explaining the error code
        B Interpretation of bank descriptor indexes (BDI) and D-bits
*/


#include	"execlib.h"



//  private methods

/* obsolete????
//  executeCM()
//
//  Sends a message to the operator console.  Allowable in inactive mode.
void
RSIManager::executeCM
(
    RSISession* const       pSession,
    const std::string&      message
) const
{
    std::string consMsg = pSession->getSessionName() + " " + message.substr( 0, 57 );
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );
    pSession->sendOutput( "-@@COMPLETE" );
}
*/


/* obsolete????
//  executeTerm()
//
//  Ends the session.  Allowable in inactive mode.
void
RSIManager::executeTERM
(
    RSISession* const       pSession
) const
{
    pSession->setState( RSISession::STATE_TERMINATE );
    return;
}
*/


//  generateSessionIds()
//
//  Generates a unique session number and session name.
//  Number will be between 1 and 999 inclusive, and name will be RSInnn.
bool
RSIManager::generateSessionIds
(
    COUNT* const        pSessionNumber,
    std::string* const  pSessionName
)
{
    *pSessionNumber = 1;
    bool done = false;
    while ( ( !done ) && ( (*pSessionNumber) < 999 ) )
    {
        CITSESSIONS its = m_Sessions.find( *pSessionNumber );
        if ( its == m_Sessions.end() )
        {
            done = true;
            std::stringstream strm;
            strm << "RSI" << std::setw( 3 ) << std::setfill( '0' ) << (*pSessionNumber);
            *pSessionName = strm.str();
        }
        else
            ++(*pSessionNumber);
    }

    return done;
}


//  generateSessionNumber()
//
//  Generates a unique session number (caller has a specific session name in mind).
//  To avoid confliction with our self-generated session numbers, we start at 1000.
bool
RSIManager::generateSessionNumber
(
    COUNT* const        pSessionNumber
)
{
    *pSessionNumber = 1000;
    bool done = false;
    while ( ( !done ) && ( (*pSessionNumber) <= 0177777 ) )
    {
        CITSESSIONS its = m_Sessions.find( *pSessionNumber );
        if ( its == m_Sessions.end() )
            done = true;
        else
            ++(*pSessionNumber);
    }

    return done;
}


//  handleSessionActive()
//
//  Normal handler for an active session - handles all regular traffic for the session
bool
RSIManager::handleSessionActive
(
    RSISession* const   pSession
)
{
#if 0 //????
    //  Check for output queued up in RunInfo
    checkPrintBuffer( pSession );

    //  See if RunInfo object is in RSITerm state - if so, release it.
    DemandRunInfo* pdri = pSession->getRunInfo();
    pdri->attach();
    if ( pdri->getState() == RunInfo::STATE_RSI_TERM )
    {
        pSession->clearRunInfo();
        pSession->setState( RSISession::STATE_TERMINATE );
        pdri->setState( RunInfo::STATE_RSI_DETACHED );
        pdri->detach();
        return true;
    }
    pdri->detach();

    //  Check for input
    SuperString input;
    if ( !pSession->readInput( &input ) )
        return timeoutCheck( pSession );

    {//  TODO:DEBUG
        std::string logMsg = "RSIManager::read input from session " + pSession->getSessionName() + ":" + input;
        SystemLog::write( logMsg );
    }
    pSession->sendOutput( input );

    //  Update input time
    timeoutReset( pSession );

    //  Look for transparent statement input
    if ( checkAndExecuteTransparentImage( pSession, input ) )
        return true;

    //  Not transparent, are we allowed to send anything to the CoarseScheduler?
    //  If so, put it into the Run's input buffer
    pdri->attach();
    if ( !pdri->isInputAllowed() )
    {
        pdri->detach();
        pSession->sendOutput( "*WAIT-LAST INPUT IGNORED*" );
        return true;
    }

    //  Put image into buffer - we assume (due to how we do RSI processing) that the
    //  input buffer is RSI, is empty, and current charset is ASCII.
    SymbiontBuffer* psbuff = pdri->getSymbiontBufferRead();
    COUNT dataWords = input.size() / 4;
    if ( input.size() % 4 )
        ++dataWords;
    COUNT reqWords = dataWords + 1;
    if ( psbuff->getRemainingWrite() < reqWords )
    {
        //TODO:RSI Hmmm. we should handle this somehow, but not now.  Later.
    }

    //  Put data image control word
    Word36 cword;

    cword.setT1( dataWords );
    cword.setS6( SymbiontBuffer::CSET_ASCII );
    psbuff->writeWord( cword );

    //  Put data words directly into buffer
    Word36* pData = psbuff->getBuffer() + psbuff->getNextWrite();
    miscStringToWord36Ascii( input, pData, dataWords );
    psbuff->advanceWriteIndex( dataWords );

    //  Block input from RSI if DEMAND, detach, and we're done.
    pdri->setInputAllowed( false );
    pdri->detach();
#endif
    return true;
}


//  handleSessionConnected()
//
//  A session was connected.  Solicit user-id and password.
bool
RSIManager::handleSessionConnected
(
    RSISession* const   pSession
)
{
    //  Reset the session
    pSession->reset();

    //  Notify console
    std::string consMsg = "RSI ";
    //TODO:RSI If this is an actual RSI session from a run, put that run's RUN-ID here...
    consMsg += pSession->getSessionName() + " - ACTV";
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );

    //  Go prompt for userid/password
    pSession->setState( RSISession::STATE_SOLICIT_LOGIN );
    return true;
}


//  handleSessionInactive()
//
//  We want to terminate, but we have displayed a termination message for the user,
//  and it is rude to kill the display before the user has a chance to read the message.
//  We will wait until the timeout expires, or the user hits ENTER.
//  When that happens, we go to INACTIVE or TERMINATE_DELETE state
bool
RSIManager::handleSessionInactive
(
    RSISession* const   pSession
)
{
#if 0 //????
    //  Special case - if the session's screen is no longer open,
    //  the user has hit the close button on the window to close it.
    //  In this case, we go straight to TERMINATE_DELETE.
    if ( !pSession->isScreenOpen() )
    {
        pSession->setState( RSISession::STATE_TERMINATE_DELETE );
        return true;
    }

    SuperString input;
    if ( !pSession->readInput( &input ) )
    {
        //  Timeout -> STATE_TERMINATE_DELETE
        if ( timeoutCheck( pSession ) )
        {
            pSession->setState( RSISession::STATE_TERMINATE_DELETE );
            return true;
        }

        return false;
    }

    //  Got input - of any kind - go to STATE_CONNECTED
    pSession->setState( RSISession::STATE_CONNECTED );
#endif
    return true;
}


//  handleSessionLoginAccepted()
//
//  Send successful login messages to terminal
bool
RSIManager::handleSessionLoginAccepted
(
    RSISession* const   pSession
)
{
    std::stringstream signonMsg;
    signonMsg << "*Emulated Exec Operating System - Version " << VERSION << " (RSI)*";
    pSession->sendOutput( signonMsg.str() );

    //TODO:RSI last login date/time message

    //  If there is automated @RUN information in the user's profile,
    //  and the user has not specified an asterisk OR his profile disallows manual @RUN images,
    //  set next state to AUTOMATED_RUN_IMAGE (which we haven't yet implemented at all)
    //  TODO:RSI For now, we always do manual @RUN's
    pSession->setState( RSISession::STATE_WAITING_FOR_RUN_IMAGE );
    return true;
}


//  handleSessionLoginSolicited()
//
//  Wait for the user to provide user-id and password, and verify it if given.
bool
RSIManager::handleSessionLoginSolicited
(
    RSISession* const   pSession
)
{
#if 0 //????
    SuperString input;
    if ( !pSession->readInput( &input ) )
        return timeoutCheck( pSession );
    pSession->sendOutput( input );

    //  Update input time
    timeoutReset( pSession );

    //  Look for transparent control images
    if ( checkAndExecuteTransparentImage( pSession, input ) )
        return true;

    //  Received input for USERID/PASSWORD CLEARANCE LEVEL - erase it from the screen and parse it here.
    pSession->sendClearScreen();
    pSession->sendOutput( "*Destroy user-id/password*" );

    bool bypassRunImage;
    SuperString strUserId;
    SuperString strPassword;
    SuperString strNewPassword;
    parseUserIdPassword( pSession, input, &bypassRunImage, &strUserId, &strPassword, &strNewPassword );

    //  Exceeded max attempts?  If so, we continue to solicit userid/password, but we ignore the input.
    if ( pSession->getUserIdErrorCount() >= m_ConfigMaxSignOnAttempts )
    {
        pSession->sendOutput( "You have entered an invalid user-id." );
        pSession->incrementUserIdErrorCount();
        std::stringstream consMsg;
        consMsg << "Sign-on security error at site: " << pSession->getSessionName()
            << " - attempt # " << std::setw( 4 ) << std::setfill( '0' ) << pSession->getUserIdErrorCount()
            << " user-id: " << strUserId;
        ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
        pConsMgr->postReadOnlyMessage( consMsg.str(), m_pExec->getRunInfo() );
        pSession->setState( RSISession::STATE_SOLICIT_LOGIN );
        return true;
    }

    //  Go validate the given userid and password(s)
    SecurityManager::UserProfile profile;
    SecurityManager* pSecMgr = dynamic_cast<SecurityManager*>( m_pExec->getManager( Exec::MID_SECURITY_MANAGER ) );
    SecurityManager::ValidationStatus secStat =
        pSecMgr->validateUser( strUserId, strPassword, strNewPassword, &profile );

    bool errorFlag = false;
    switch ( secStat )
    {
    case SecurityManager::VST_INCORRECT_PASSWORD:
        pSession->sendOutput( "You have entered an invalid user-id." );
        errorFlag = true;
        break;

    case SecurityManager::VST_INCORRECT_USER_NAME:
        pSession->sendOutput( "You have entered an invalid password." );
        errorFlag = true;
        break;

    case SecurityManager::VST_SUCCESSFUL:
        break;

    case SecurityManager::VST_SUCCESSFUL_PASSWORD_CHANGED:
        pSession->sendOutput( "Your password has been replaced." );
        break;
    }

    if ( errorFlag )
    {
        pSession->incrementUserIdErrorCount();
        pSecMgr->userLoginFailure( strUserId );

        std::stringstream consMsg;
        consMsg << "Sign-on security error at site: " << pSession->getSessionName()
            << " - attempt # " << std::setw( 4 ) << std::setfill( '0' ) << pSession->getUserIdErrorCount()
            << " user-id: " << strUserId;
        ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
        pConsMgr->postReadOnlyMessage( consMsg.str(), m_pExec->getRunInfo() );
        pSession->setState( RSISession::STATE_SOLICIT_LOGIN );

        if ( pSession->getUserIdErrorCount() >= m_ConfigMaxSignOnAttempts )
        {
            std::string logStr = "Session " + pSession->getSessionName() + " reached max attempts";
            SystemLog::write( logStr );
        }

        return true;
    }

    //  Make sure the given user-id has access to DEMAND mode.
    if ( !profile.m_CanAccessDemand )
    {
        pSession->sendOutput( "Your user-id has no access to demand." );
        pSession->setState( RSISession::STATE_TERMINATE );
        return true;
    }

    //  Update session information
    pSession->setUserId( profile.m_UserId );
    pSession->setLoginProfile( profile );
    pSession->setTimeoutSecs( profile.m_DemandTimeoutSeconds );
    pSession->setBypassRunCard( bypassRunImage && profile.m_CanBypassRunCard );

    //  Notify security system of successful login
    pSecMgr->userLoginSuccess( strUserId );

    pSession->setState( RSISession::STATE_LOGIN_ACCEPTED );
#endif
    return true;
}


//  handleSessionSolicitLogin()
//
//  Either we just got connected, or a login attempt failed and we're back here for another try
bool
RSIManager::handleSessionSolicitLogin
(
    RSISession* const   pSession
)
{
    //  Make sure there is no hold on Demand terminals
    if ( m_pExec->areDemandTerminalsHeld() )
    {
        pSession->sendOutput( "*TERMINATED DUE TO SYSTEM HOLD ON TERMINALS*" );
        pSession->setState( RSISession::STATE_TERMINATE );
        return true;
    }

    //  Check limitations (GCCMIN and RSICNT)
    if ( m_Sessions.size() > m_ConfigMaxSessions )
    {
        pSession->sendOutput( "*TERMINATED DUE TO UNAVAILABLE SYSTEM RESOURCES*" );
        pSession->setState( RSISession::STATE_TERMINATE );
        return true;
    }

    //  Prompt for userid/password
    pSession->sendOutput( "Enter your user-id/password:" );
    pSession->setState( RSISession::STATE_LOGIN_SOLICITED );
    return true;
}


//  handleSessionTerminate()
//
//  Session needs to terminate.  All code which wants to terminate the session must call here,
//  and from here we decide which subsequent path to take.
bool
RSIManager::handleSessionTerminate
(
    RSISession* const   pSession
)
{
    //  Is there an activity (for an @@ statement)?  If so, stop it and wait for it to go away.
    TransparentActivity* pAct = pSession->getTransparentActivity();
    if ( pAct )
    {
        pAct->stop( Activity::STOP_DEMAND_TERM, false );
        pSession->setState( RSISession::STATE_TERMINATE_WAIT_ACT );
        return true;
    }

    //  Is there a RunInfo?  If so, terminate it.
    ControlModeRunInfo* pRunInfo = pSession->getRunInfo();
    if ( pRunInfo )
    {
        m_pExec->terminateDemandRun( dynamic_cast<DemandRunInfo*>( pRunInfo ) );
        pSession->setState( RSISession::STATE_TERMINATE_WAIT_RUNINFO );
        return true;
    }

    //  No RunInfo (or it went away, and we're back here again).
    //  Send something to the user, and go to STATE_INACTIVE
    pSession->sendOutput( "*TERMINAL INACTIVE*" );
    pSession->updateLastInputTime();
    pSession->setState( RSISession::STATE_INACTIVE );
    return true;
}


//  handleSessionTerminateDelete()
//
//  Session has terminated, and we're ready to delete the thing.
bool
RSIManager::handleSessionTerminateDelete
(
    RSISession* const   pSession
)
{
    //  Send console message
    std::string consMsg = "RSI " + pSession->getSessionName() + " - ";
    if ( pSession->getTimeoutFlag() )
        consMsg += "TIMEOUT";
    else
        consMsg += "INACTIVE";
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );

    //  Delete the session
    ITSESSIONS itsess = m_Sessions.find( pSession->getSessionNumber() );
    assert( itsess != m_Sessions.end() );

    delete pSession;
    m_Sessions.erase( itsess );

    return true;
}


//  handleSessionTerminateWaitAct()
//
//  We want to terminate, but there is a transparent activity outstanding.
//  Check on it, and when it terminates, change state back to TERMINATE.
bool
RSIManager::handleSessionTerminateWaitAct
(
    RSISession* const   pSession
)
{
    //  See if the activity has completely gone away - it shouldn't, but just in case...
    TransparentActivity* pAct = pSession->getTransparentActivity();
    if ( !pAct )
    {
        pSession->setState( RSISession::STATE_TERMINATE );
        return true;
    }

    //  Check whether the activity is now terminated...  If so, delete it.
    if ( pAct->isTerminated() )
    {
        pSession->unregisterTransparentActivity();
        return true;
    }

    return false;
}


//  handleSessionTerminateWaitRunInfo()
//
//  We want to terminate, but there is a RunInfo object outstanding.
//  If the session's runinfo pointer goes away, or the runinfo goes to ST_RSI_DETACHED,
//  we can go back to STATE_TERMINATE status.
bool
RSIManager::handleSessionTerminateWaitRunInfo
(
    RSISession* const   pSession
)
{
    DemandRunInfo* pdri = pSession->getRunInfo();
    if ( pdri )
    {
        pdri->attach();
        if ( pdri->getState() == RunInfo::STATE_RSI_TERM )
        {
            pSession->clearRunInfo();
            pSession->setState( RSISession::STATE_TERMINATE );
            pdri->setState( RunInfo::STATE_RSI_DETACHED );
            pdri->detach();
            pdri = 0;
        }
        else
            pdri->detach();
    }

    if ( pdri == 0 )
        pSession->setState( RSISession::STATE_TERMINATE );

    return false;
}


//  handleSessionWaitingForRunImage()
//
//  Credentials were accepted, but there is no automatically-generated @RUN image.
//  Wait for user to enter said image.
bool
RSIManager::handleSessionWaitingForRunImage
(
    RSISession* const   pSession
)
{
#if 0 //????
    SuperString input;
    if ( !pSession->readInput( &input ) )
        return timeoutCheck( pSession );
    pSession->sendOutput( input );

    //  Update input time
    timeoutReset( pSession );

    //  Look for transparent statement input
    if ( checkAndExecuteTransparentImage( pSession, input ) )
        return true;

    //  Look for @RUN image.  Use RSIActivity's credentials and exec RunInfo, since user's RunInfo isn't set up yet.
    CSInterpreter* pcsi = new CSInterpreter( m_pExec,
                                             m_pActivity,
                                             m_pExec->getRunInfo()->getSecurityContext(),
                                             m_pExec->getRunInfo() );
    CSInterpreter::Status csiStat = pcsi->interpretStatement( input, true, false, false );
    if ( pcsi->getCommand() != CSInterpreter::CMD_RUN )
    {
        pSession->sendOutput( "*NO RUN ACTIVE*" );
        delete pcsi;
        return true;
    }

    if ( csiStat == CSInterpreter::CSIST_ILLEGAL_OPTION )
    {
        std::stringstream strm;
        strm << std::setw( 2 ) << pcsi->getIndex() + 1 << " ILLEGAL OPTION ";
        pSession->sendOutput( strm.str() );
        delete pcsi;
        return true;
    }
    else if ( csiStat != CSInterpreter::CSIST_SUCCESSFUL )
    {
        pSession->sendOutput( "BAD RUN STATEMENT" );
        delete pcsi;
        return true;
    }

    //  If option B is given, we divert to batch run sequence
    //  TODO:BATCH "*BATCH RUN MODE - ENTER RUN STREAM*" (note: do we even support this?)

    DemandRunInfo* pDRInfo = m_pExec->createDemandRun( m_pActivity,
                                                       pSession->getLoginProfile(),
                                                       pSession->getSessionNumber(),
                                                       pSession->getSessionName(),
                                                       pcsi->getRunId(),
                                                       pcsi->getAccountId(),
                                                       pcsi->getUserId(),
                                                       pcsi->getProjectId(),
                                                       pcsi->getOptions(),
                                                       pcsi->getSchedulingPriority(),
                                                       pcsi->getProcessorDispatchingPriority() );
    delete pcsi;
    if ( pDRInfo == 0 )
        return true;
    pSession->setRunInfo( pDRInfo );

    //  Put date/time message into PRINT$
    //  Format: 'DATE: 022487 TIME: 000055'
    SYSTEMTIME systemTime;
    GetLocalTime( &systemTime );
    std::stringstream dateTimeStrm;
    dateTimeStrm << "DATE: ";
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << systemTime.wMonth;
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << systemTime.wDay;
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << ( systemTime.wYear % 100 );
    dateTimeStrm << " TIME: ";
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << systemTime.wHour;
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << systemTime.wMinute;
    dateTimeStrm << std::setw(2) << std::setfill( '0' ) << systemTime.wSecond;
    pDRInfo->attach();
    pDRInfo->postToPrint( dateTimeStrm.str() );
    pDRInfo->detach();

    //  Move on to the next step
    pSession->setState( RSISession::STATE_ACTIVE );
#endif
    return true;
}



//  private, protected statics

//  checkAndExecuteTransparentImage()
//
//  Checks the given input to see whether it is an attempt at a transparent control statement.
//  If so, we validate it, and if it's good, we try to execute it.
//
//  Returns:
//      true if we got a transparent control statement
//          this includes bad and mis-formatted statements, as well as good ones,
//          and includes successful and unsuccessful execution of the good ones.
//      false if we don't have anything resembling a transparent control statement
bool
RSIManager::checkAndExecuteTransparentImage
(
    RSISession* const       pSession,
    const std::string&      image
)
{
    //  Create an interpreter object, and interpret the input.
    TransparentCSInterpreter* pTCSInt = new TransparentCSInterpreter( image );
    TransparentCSInterpreter::Status tcsStat = pTCSInt->interpretStatement();
    if ( tcsStat == TransparentCSInterpreter::TCSIST_NOT_FOUND )
    {
        delete pTCSInt;
        return false;
    }

    //  Syntax error?
    if ( tcsStat == TransparentCSInterpreter::TCSIST_SYNTAX_ERROR )
    {
        pSession->sendOutput( "-@@ERROR - SYNTAX" );
        delete pTCSInt;
        return true;
    }

    //  If there is a long-running @@ command already in progress, complain here.
    TransparentActivity* pAct = pSession->getTransparentActivity();
    if ( pAct )
    {
        pSession->sendOutput( "-@@IMAGE IGNORED - TRANSP CTL IN PROGRESS" );
        delete pTCSInt;
        return true;
    }

    //  Create a TransparentActivity to handle the statement, and start it.
    pSession->registerTransparentActivity( pTCSInt );
    return true;
}


//  checkPrintBuffer()
//
//  Checks the PRINT$ output buffer from RunInfo (which will be an RSISymbiontBuffer)
//  to see if there is any output which we need to pull and process.
void
RSIManager::checkPrintBuffer
(
    RSISession* const   pSession
)
{
    ControlModeRunInfo* pri = pSession->getRunInfo();
    pri->attach();

    //  Check for output queued up in RunInfo - we can process multiple images.
    SymbiontBuffer* psbuff = pri->getSymbiontBufferPrint();
    if ( psbuff )
    {
        while ( !psbuff->isExhausted() )
        {
            const Word36* pCWord = psbuff->readWord();
            if ( pCWord->isNegative() )
            {
                //  Control image
                BYTE controlCode = pCWord->getS1();
                COUNT dataWords = pCWord->getS2();
                //TODO:RSI sanity check datawords
                switch ( controlCode )
                {
                case 040:   //  Bypass
                case 043:   //  Fortran V backspace
                case 052:   //  SIR$ Line-change
                case 053:   //  CTS/IPF Line number
                    break;

                case 051:   //  Continuation
                case 054:   //  End of reel
                    assert( false );
                    break;

                case 042:   //  Charset change
                    psbuff->setCurrentCharacterSet( static_cast<SymbiontBuffer::CharacterSet>( pCWord->getS6() ) );
                    break;

                case 050:   //  Label
                    psbuff->setSDFType( static_cast<SymbiontBuffer::SDFType>( pCWord->getS3() ) );
                    psbuff->setCurrentCharacterSet( static_cast<SymbiontBuffer::CharacterSet>( pCWord->getS6() ) );
                    break;

                case 060:   //  print control
                case 061:   //  special print control
                case 070:   //  punch control
                    //TODO:RSI stuff to do
                    break;

                case 077:   //  end-of-file - TODO:RSI hould we expect this?
                    assert( false );
                    break;
                }

                psbuff->advanceReadIndex( dataWords );
            }

            else
            {
                //  Data image
                COUNT dataWords = pCWord->getT1();
                //TODO:RSI sanity check datawords
                Word36* pData = psbuff->getBuffer() + psbuff->getNextRead();
                std::string strImage = miscWord36AsciiToString( pData, dataWords, false );
                pSession->sendOutput( strImage );
                psbuff->advanceReadIndex( dataWords );
            }
        }

        psbuff->clear();
    }

    pri->detach();
}


//  parseUserIdPassword
//
//  Parses the input string the user gives us after being prompted for userid/password.
//  Format is:
//      ['*'] {userId} ['/' {password} ['/' {newPassword}] ]
//
//  userId:
//      1 to 12 characters, any combination of letters, numbers, '-' and '.'.
//
//  password, newPassword:
//      1 to 12 characters, excluding space, comma, and slash.
void
RSIManager::parseUserIdPassword
(
    RSISession* const   pSession,
    const SuperString&  input,
    bool* const         pBypassRunImage,
    SuperString* const  pUserId,
    SuperString* const  pPassword,
    SuperString* const  pNewPassword
)
{
    pUserId->clear();
    pPassword->clear();
    pNewPassword->clear();
    (*pBypassRunImage) = false;

    //  Asterisk?
    INDEX ix = 0;
    if ( input[ix] == '*' )
    {
        (*pBypassRunImage) = true;
        ++ix;
    }

    while ( (ix < input.size()) && (input[ix] != '/') )
        (*pUserId) += input[ix++];

    if ( input[ix] == '/' )
    {
        ++ix;
        while ( (ix < input.size()) && (input[ix] != '/') )
            (*pPassword) += input[ix++];
    }

    if ( input[ix] == '/' )
    {
        ++ix;
        while ( (ix < input.size()) && (input[ix] != '/') )
            (*pNewPassword) += input[ix++];
    }
}


//  timeoutCheck()
//
//  Compares current time to session's last input time, and issues timeout warnings
//  and makes state changes as appropriate.
//
//  Returns:
//      true if we've changed the state (caller must react accordingly),
//      otherwise false
bool
RSIManager::timeoutCheck
(
    RSISession* const       pSession
)
{
    //  Only test if the configured timeout is non-zero - zero means infinite time allowed.
    if ( pSession->getTimeoutSecs() > 0 )
    {
        COUNT64 systemTime = SystemTime::getMicrosecondsSinceEpoch();
        COUNT64 elapsedSeconds = (systemTime - pSession->getLastInputSystemTime()) / SystemTime::MICROSECONDS_PER_SECOND;

        if ( elapsedSeconds >= pSession->getTimeoutSecs() )
        {
            pSession->sendOutput( "*SESSION TIMED OUT*" );
            pSession->setState( RSISession::STATE_TERMINATE );
            return true;
        }

        COUNT64 remainingSeconds = pSession->getTimeoutSecs() - elapsedSeconds;
        if ( ( pSession->getTimeoutSecs() ) > 60
            && ( !pSession->getTimeoutWarning() )
            && ( remainingSeconds < 60 ) )
        {
            pSession->sendOutput( "*TIMEOUT WARNING*" );
            pSession->setTimeoutWarning( true );
        }
    }

    return false;
}


//  timeoutReset()
//
//  Resets timeout processing for a session, probably as a result of reading input
void
RSIManager::timeoutReset
(
    RSISession* const       pSession
)
{
    pSession->updateLastInputTime();
    pSession->setTimeoutWarning( false );
}


//  Constructors, destructors

RSIManager::RSIManager
(
    Exec* const         pExec
)
:ExecManager( pExec )
{
}



//  Public methods

//  cleanup()
//
//  ExecManager interface
//  Called just before this object is deleted
void
RSIManager::cleanup()
{
    //  Nothing to do, I think...
}


//  dump()
//
//  ExecManager interface
//  For debugging
void
RSIManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "RSIManager ----------" << std::endl;
    lock();
    for ( CITSESSIONS itsess = m_Sessions.begin(); itsess != m_Sessions.end(); ++itsess )
        itsess->second->dump( stream, "  ", dumpBits );
    unlock();
}


//  getUserId()
//
//  Retrieves the user id for the session
RSIManager::Status
RSIManager::getUserId
(
    const COUNT         sessionNumber,
    std::string* const  pUserId
) const
{
    Status stat = ST_SUCCESSFUL;
    lock();

    CITSESSIONS itsess = m_Sessions.find( sessionNumber );
    if ( (itsess == m_Sessions.end()) || itsess->second->isTerminating() || (itsess->second->getState() == RSISession::STATE_INACTIVE) )
        stat = ST_NO_DEVICE;
    else
        *pUserId = itsess->second->getUserId();

    unlock();
    return stat;
}


#if 0 //????
//  openTerminalSession()
//
//  Creates a new Session object and connects the screen, this manager,
//  and the EXEC, in order to provide a DEMAND user session.
bool
RSIManager::openSession
(
    SmartConsole* const         pScreen,
    const std::string* const    pSiteId     // defaults to 0, to use generated ID
)
{
    lock();

    //  Is the EXEC running?  if not, don't allow this.
    if ( m_pExec->getStatus() == Exec::ST_STOPPED )
    {
        unlock();
        return false;
    }

    //  Generate unique session info (if possible)
    COUNT sessionNumber;
    std::string sessionName;
    if ( pSiteId )
    {
        //  Verify the requested ID is not duplicated.
        //  If we do get a match, emit "RSI site-id DUPLICATED" to the console
        //TODO:RSI and do not allow to continue

        //  Caller wants a specific site-id.  Accomodate him if possible.
        if ( !generateSessionNumber( &sessionNumber ) )
        {
            //  Cannot generate unique number (very rare for us, but possible I suppose)
            //TODO:RSI do something (do not continue)
        }
    }
    else
    {
        if ( !generateSessionIds( &sessionNumber, &sessionName ) )
        {
            //  Cannot generate unique IDs (very rare for us, but possible I suppose)
            //TODO:RSI do something (do not continue)
        }
    }

    m_Sessions[sessionNumber] = new RSISession( m_pExec, sessionNumber, sessionName, pScreen );
    unlock();
    return true;
}
#endif


//  poll()
//
//  Invoked by RSIActivity on a frequent basis to keep RSI communication flowing
//
//  Returns:
//      true if we did something that would argue in favor of another immediate poll,
//      false if caller should wait a little bit before the next poll
bool
RSIManager::poll()
{
    lock();

    bool repoll = false;

    ITSESSIONS itsess = m_Sessions.begin();
    while ( itsess != m_Sessions.end() )
    {
        //  Preserve pointer to *next* Session, in case *this* one needs to go away.
        ITSESSIONS itnext = itsess;
        ++itnext;

        //  Check session's @@ activity - if there is one, and it's stale, lose it.
        RSISession* pSession = itsess->second;
        if ( ( pSession->getTransparentActivity() ) && ( pSession->getTransparentActivity()->isTerminated() ) )
            pSession->unregisterTransparentActivity();

        //  Now deal with the session according to its state...
        bool didSomething = false;
        switch ( pSession->getState() )
        {
        case RSISession::STATE_ACTIVE:
            didSomething = handleSessionActive( pSession );
            break;

        case RSISession::STATE_CONNECTED:
            didSomething = handleSessionConnected( pSession );
            break;

        case RSISession::STATE_INACTIVE:
            didSomething = handleSessionInactive( pSession );
            break;

        case RSISession::STATE_LOGIN_ACCEPTED:
            didSomething = handleSessionLoginAccepted( pSession );
            break;

        case RSISession::STATE_LOGIN_SOLICITED:
            didSomething = handleSessionLoginSolicited( pSession );
            break;

        case RSISession::STATE_SOLICIT_LOGIN:
            didSomething = handleSessionSolicitLogin( pSession );
            break;

        case RSISession::STATE_TERMINATE:
            didSomething = handleSessionTerminate( pSession );
            break;

        case RSISession::STATE_TERMINATE_DELETE:
            didSomething = handleSessionTerminateDelete( pSession );
            break;

        case RSISession::STATE_TERMINATE_WAIT_RUNINFO:
            didSomething = handleSessionTerminateWaitRunInfo( pSession );
            break;

        case RSISession::STATE_WAITING_FOR_RUN_IMAGE:
            didSomething = handleSessionWaitingForRunImage( pSession );
            break;

        case RSISession::STATE_TERMINATE_WAIT_ACT://???? what do do
            break;
        }

        if ( didSomething )
            repoll = true;

        //  Next session
        itsess = itnext;
    }

    unlock();
    return repoll;
}


//  sendOutput()
//
//  Sends output directly to the RSI session - for external users
RSIManager::Status
RSIManager::sendOutput
(
    const COUNT         sessionNumber,
    const std::string&  text,
    const bool          force   //  sends output even if we are in @@HOLD
)
{
    Status stat = ST_SUCCESSFUL;
    lock();

    ITSESSIONS itsess = m_Sessions.find( sessionNumber );
    if ( (itsess == m_Sessions.end()) || itsess->second->isTerminating() )
        stat = ST_NO_DEVICE;
    else
    {
        bool result = itsess->second->sendOutput( text );
        if ( !result )
            stat = ST_NOT_READY;
    }

    unlock();
    return stat;
}


//  shutdown()
//
//  ExecManager interface
//  Exec wants to shut down, but activities are still active.
void
RSIManager::shutdown()
{
    SystemLog::write("RSIManager::shutdown()");
    //  Nothing to do right now.
}


//  startup()
//
//  ExecManager interface
//  Initializes the manager.
bool
RSIManager::startup()
{
    SystemLog::write("RSIManager::startup()");
    lock();

    //  Go get any interesting configuration values
    m_ConfigMaxSignOnAttempts = m_pExec->getConfiguration().getIntegerValue( "MAXATMP" );
    m_ConfigMaxSessions = m_pExec->getConfiguration().getIntegerValue( "RSICNT" );

    //  Put all existing sessions into initial state
    for ( ITSESSIONS itsess = m_Sessions.begin(); itsess != m_Sessions.end(); ++itsess )
    {
        RSISession* pSession = itsess->second;
        pSession->reset();
    }

    unlock();
    return true;
}


//  terminate()
//
//  ExecManager interface
//  Final termination of exec.
//  All activities are gone, so we can close out whatever else we need to.
void
RSIManager::terminate()
{
    SystemLog::write("RSIManager::terminate()");
    //  Nothing to do right now.
}


//  terminateSession()
//
//  Called by TransparentActivity in response to @@TERM, to set session state.
RSIManager::Status
RSIManager::terminateSession
(
    const COUNT         sessionNumber
)
{
    Status stat = ST_SUCCESSFUL;
    lock();

    ITSESSIONS itsess = m_Sessions.find( sessionNumber );
    if ( (itsess == m_Sessions.end()) || itsess->second->isTerminating() )
        stat = ST_NO_DEVICE;
    else
        itsess->second->setState( RSISession::STATE_TERMINATE );

    unlock();
    return stat;
}

