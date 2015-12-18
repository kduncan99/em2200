//  Exec class implementation



#include    "execlib.h"



//  statics

static const char * DayTable[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char * MonthTable[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

//  Characters which are valid for read and write keys
bool ValidKeyCharacters[256] =
{
    //  control codes
    false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,
    false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,
    //  space !     "       #       $       %       &       '       (       )       *       +       ,       -       .       /
    false,  true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   false,  true,   false,  false,
    // 0    1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
    true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   false,  true,   true,   true,   true,
    // @    A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
    true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,
    // P    Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
    true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,
    // `    a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
    false,  true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,
    // p    q       r       s       t       u       v       w       x       y       z       {       |       }       ~
    true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,   false,  false,  false,  false,  false,
};



//  private methods

//  abortRun()
//
//  Common handling for aborting a run.
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo of affected run
void
Exec::abortRun
(
    RunInfo* const pRunInfo
)
{
    //  Hopefully this never happens for the EXEC itself
    if ( pRunInfo->isExec() )
    {
        stopExec( Exec::SC_EXEC_IN_CONTINGENCY );
        return;
    }

    if ( pRunInfo->getStatus() != RunInfo::STATUS_ABORT )
    {
        ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
        std::string consStr = pRunInfo->getActualRunId() + " ABORT";
        pcmgr->postReadOnlyMessage( consStr, pRunInfo );
        pRunInfo->setStatus( RunInfo::STATUS_ABORT );
    }
}


//  assignDIAGToRun()
//
//  Assigns a temporary DIAG$ file to the given run; handles failures appropriately
//
//  Parameters:
//      pActivity:              pointer to requesting Activity
//      pRunInfo:               pointer to user's RunInfo object
bool
Exec::assignDIAG$ToRun
(
    Activity* const         pActivity,
    UserRunInfo* const      pRunInfo
)
{
    SuperString fileType = m_pConfiguration->getStringValue( "MDFALT" );
    COUNT fileMaxSize = m_pConfiguration->getIntegerValue( "MAXGRN" );

    std::stringstream facStrm;
    facStrm << "@ASG,T DIAG$.," << fileType << "/0/TRK/" << fileMaxSize;

    return assignFileToRun( pActivity, pRunInfo, facStrm.str(), "DIAG$" );
}


//  assignFileToRun()
//
//  Common code for separate functions which assign TPF$, DIAG$, etc
//
//  Parameters:
//      pActivity:          pointer to requesting Activity
//      pRunInfo:           pointer to user's RunInfo object
//      assignImage:        @ASG image
//      fileName:          pointer to descriptive file name for error message, if needed
bool
Exec::assignFileToRun
(
    Activity* const         pActivity,
    UserRunInfo* const      pRunInfo,
    const std::string&      assignImage,
    const std::string&      fileName
)
{
    bool result = true;

    CSInterpreter* pcsi = new CSInterpreter( this, pActivity, pRunInfo->getSecurityContext(), pRunInfo );
    CSInterpreter::Status csiStat = pcsi->interpretStatement( assignImage, true, false, false );
    if ( csiStat == CSInterpreter::CSIST_SUCCESSFUL )
        csiStat = pcsi->executeStatement( pActivity );

    //  csiStat *should* be CSIST_FACILITIES_RESULT, and the fac status should have no errors.
    //  If it is CSIST_SUCCESSFUL, something is off, but we won't complain about it.
    if ( csiStat == CSInterpreter::CSIST_FACILITIES_RESULT )
    {
        FacilitiesManager::Result facResult = pcsi->getFacilitiesResult();
        if ( facResult.containsErrorStatusCode() )
        {
            std::stringstream errStrm;
            errStrm << fileName << " FILE ASSIGN ERROR";
            errStrm << " - " << std::oct << std::setw( 6 ) << std::setfill( '0' ) << facResult.m_StatusCodeInstances[0].m_StatusCode;
            pRunInfo->postToPrint( errStrm.str() );
            pRunInfo->setErrorMode();
            pRunInfo->setState( RunInfo::STATE_FIN );
            result = false;
        }
    }

    return result;
}


//  assignTPFToRun()
//
//  Assigns a temporary TPF$ file to the given run; handles failures appropriately
//
//  Parameters:
//      pActivity:              pointer to requesting Activity
//      pRunInfo:               pointer to user's RunInfo object
bool
Exec::assignTPF$ToRun
(
    Activity* const         pActivity,
    UserRunInfo* const      pRunInfo
)
{
    SuperString fileType = m_pConfiguration->getStringValue( "TPFTYP" );
    COUNT fileMaxSize = m_pConfiguration->getIntegerValue( "TPFMAXSIZ" );

    std::stringstream facStrm;
    facStrm << "@ASG,T TPF$.," << fileType << "/0/TRK/" << fileMaxSize;

    return assignFileToRun( pActivity, pRunInfo, facStrm.str(), "TPF$" );
}


//  generateUniqueRunId()
//
//  Establishes the actual runid based on the given original runid.
//  We have a slightly different algorithm than the traditional EXEC.
//  TODO:BATCH Need to do what the EXEC does, then take a SC_RUNID_FAILURE if we cannot generate a unique ID
//          and after sending "UNABLE TO GENERATE UNIQUE RUNID FOR run-id"
//
//  Parameters:
//      originalRunId:      original Run ID
//      pUniqueRunId:       pointer to where we store the generated runId
//
//  Returns:
//      true generally, false if we cannot generate a unique run-id (too many exist, or whatever)
bool
Exec::generateUniqueRunId
(
    const std::string&      originalRunId,
    std::string* const      pUniqueRunId
) const
{
    std::string uniqueRunId = originalRunId;

    //  There is an outside chance that the algorithm below could run without stopping...
    //  if all possible runid's already exist.
    if ( ( m_RunInfoTable.find( uniqueRunId ) != m_RunInfoTable.end() ) && ( uniqueRunId.size() < 6 ) )
        uniqueRunId += 'A';
    while ( m_RunInfoTable.find( uniqueRunId ) != m_RunInfoTable.end() )
    {
        // Can we increment the last character of the run id?
        INDEX lastx = uniqueRunId.size() - 1;
        char lastCh = uniqueRunId[lastx];
        if ( lastCh == 'Z' )
        {
            uniqueRunId[lastx] = '0';
            continue;
        }
        if ( lastCh != '9' )
        {
            uniqueRunId[lastx] = lastCh + 1;
            continue;
        }

        // Nope.  If we're less than six characters, append an 'A'.
        if ( lastx < 5 )
        {
            uniqueRunId += 'A';
            continue;
        }

        // Otherwise, do a right-to-left increment on the entire 6 characters.
        INDEX cx = 6;
        while ( cx > 0 )
        {
            --cx;
            char ch = uniqueRunId[cx];
            if ( ch == 'Z' )
            {
                uniqueRunId[cx] = '0';
                break;
            }
            if ( ch != '9' )
            {
                uniqueRunId[cx] = ch + 1;
                break;
            }
            uniqueRunId[cx] = 'A';
        }
    }

    if ( uniqueRunId.compare( originalRunId ) != 0 )
    {
        //  run-id DUPLICATED NEW ID IS run-idx
        std::string str = originalRunId;
        str += " Duplicated new id is ";
        str += uniqueRunId;
        ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
        pcmgr->postReadOnlyMessage( str, m_pRunInfo );
    }

    *pUniqueRunId = uniqueRunId;
    return true;
}


//  terminateExecActivities()
//
//  Terminates all exec activities
void
Exec::terminateExecActivities() const
{
    m_pRunInfo->attach();
    m_pRunInfo->killActivities();

    while ( m_pRunInfo->hasLiveActivity() )
    {
        m_pRunInfo->detach();
        miscSleep( 100 );
        m_pRunInfo->attach();
    }

    m_pRunInfo->detach();
}



//  private static methods

//  getTimeStamp()
//
//  For time-stamping console START and FIN messages - format is always HH:MM:SS
inline std::string
Exec::getTimeStamp()
{
    SystemTime* pLocalTime = SystemTime::createLocalSystemTime();
    std::stringstream strm;
    strm << std::setw( 2 ) << std::setfill( '0' ) << pLocalTime->getHour()
            << ":" << pLocalTime->getMinute()
            << ":" << pLocalTime->getSecond();
    delete pLocalTime;
    return strm.str();
}


//  worker()
//
//  Asynchronous worker thread.
//  Handles console input, and anything else we can think of.
void
Exec::worker()
{
    m_pPanelInterface->setStatusMessage( getStatusString( m_Status ));

    //  Kill any RunInfo objects still hanging around from a previous session
    while ( m_RunInfoTable.size() > 0 )
    {
        delete m_RunInfoTable.begin()->second;
        m_RunInfoTable.erase( m_RunInfoTable.begin()->first );
    }

    if ( m_Status == ST_NOT_BOOTED )
    {
        m_CurrentSession = 1;
        m_InitialBoot = true;
    }
    else
    {
        ++m_CurrentSession;
        m_InitialBoot = false;
    }

    setStatus( ST_INIT );

    //  Create RunInfo object for the Exec
    std::string overheadAccountId = m_pConfiguration->getStringValue( "OVRACC" );
    std::string overheadUserId = m_pConfiguration->getStringValue( "OVRUSR" );
    m_pRunInfo = new ExecRunInfo( this, overheadAccountId, overheadUserId );
    m_pRunInfo->setSecurityContext( new ExecSecurityContext() );
    m_RunInfoTable.insert( std::make_pair( m_pRunInfo->getActualRunId(), m_pRunInfo ) );

    //  Wake up the managers
    bool startError = false;
    for ( ITEXECMANAGERS itm = m_Managers.begin(); itm != m_Managers.end(); ++itm )
    {
        if ( !(*itm)->startup() )
        {
            SystemLog::write( "Exec::worker():Could not start a manager" );
            stopExec( Exec::SC_START_ERROR );
            startError = true;
            break;
        }
    }

    if ( !startError )
    {
        //  Start IoActivity
        IoManager* piomgr = dynamic_cast<IoManager*>( m_Managers[MID_IO_MANAGER] );
        IoActivity* pIoActivity = new IoActivity( this, piomgr );
        m_pRunInfo->appendTaskActivity( pIoActivity );
        pIoActivity->start();
        while ( !pIoActivity->isReady() )
            workerWait( 100 );

        //  Start ConsoleActivity
        ConsoleManager* pconsmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
        ConsoleActivity* pConsActivity = new ConsoleActivity( this, pconsmgr );
        m_pRunInfo->appendTaskActivity( pConsActivity );
        pConsActivity->start();
        while ( !pConsActivity->isReady() )
            workerWait( 100 );

        //  Start BootActivity
        BootActivity* pBootActivity = new BootActivity( this );
        m_pRunInfo->appendTaskActivity( pBootActivity );
        pBootActivity->start();

        while ( !isWorkerTerminating() )
        {
            //  Do nothing until the world ends...
            miscSleep( 1000 );
        }

        //  Set Exec status, and kill all user-level activities
        setStatus( ST_TERMINATING );

        //  Kill any remaining user activities.
        //  Don't delete anything - the user might want to dump the Exec.
        lock();
        for ( ITRUNINFOS itri = m_RunInfoTable.begin(); itri != m_RunInfoTable.end(); ++itri )
        {
            if ( !itri->second->isExec() )
                itri->second->killActivities();
        }
        unlock();
    }

    //  Alert the managers, so they will release any user (or Exec) activities which are hung
    //  waiting for a manager to do something.
    for ( ITEXECMANAGERS item = m_Managers.begin(); item != m_Managers.end(); ++item )
        (*item)->shutdown();

    //  Display system stop message ONLY if auto-recovery is inhibited
    if ( m_pPanelInterface->getJumpKey( 3 ) )
        displaySystemErrorMsg( m_LastStopCode, m_CurrentSession );

    //  Now terminate the exec activities
    terminateExecActivities();

    //  Terminate the managers
    for ( ITEXECMANAGERS item = m_Managers.begin(); item != m_Managers.end(); ++item )
        (*item)->terminate();

    //  All done.
    setStatus( ST_STOPPED );
}


//  workerStart()
//
//  Nobody should call this
bool
Exec::workerStart()
{
    assert( false );
    return false;
}


//  workerStop()
//
//  Nobody should call this
bool
Exec::workerStop
(
    const bool
)
{
    assert( false );
    return false;
}



//  constructors, destructors

Exec::Exec
(
    PanelInterface* const   pPanelInterface,
    Configuration* const    pConfiguration,
    NodeTable* const        pNodeTable
)
    :Worker( "Exec" ),
    m_pConfiguration( pConfiguration ),
    m_pNodeTable( pNodeTable ),
    m_pPanelInterface( pPanelInterface )
{
    m_CurrentSession = 0;
    m_ExecTimeOffsetMicros = 0;
    m_HoldBatchRuns = false;
    m_HoldDemandRuns = false;
    m_HoldDemandTerminals = false;
    m_InitialBoot = true;
    m_LastStopCode = SC_NONE;
    m_OperatorBoot = false;
    m_pRunInfo = 0;
    m_Status = ST_NOT_BOOTED;

	// Create the Managers
    m_Managers.resize( m_ManagerCount );
    m_Managers[MID_DEVICE_MANAGER] = new DeviceManager( this );
    m_Managers[MID_IO_MANAGER] = new IoManager( this );
    m_Managers[MID_CONSOLE_MANAGER] = new ConsoleManager( this );
    m_Managers[MID_MFD_MANAGER] = new MFDManager( this );
    m_Managers[MID_FACILITIES_MANAGER] = new FacilitiesManager( this );
    m_Managers[MID_QUEUE_MANAGER] = new QueueManager( this );
    m_Managers[MID_ACCOUNT_MANAGER] = new AccountManager( this );
    m_Managers[MID_SECURITY_MANAGER] = new SecurityManager( this );
    m_Managers[MID_RSI_MANAGER] = new RSIManager( this );
}


Exec::~Exec()
{
	// Delete the managers
	while ( m_Managers.size() )
	{
		delete m_Managers.back();
		m_Managers.pop_back();
	}

    // Remove any existing RunInfo objects
    m_pRunInfo = 0;
    while ( m_RunInfoTable.size() != 0 )
    {
        delete m_RunInfoTable.begin()->second;
        m_RunInfoTable.erase( m_RunInfoTable.begin()->first );
    }
}



//  public methods

//  abortRunDemand()
//
//  Aborts a run due to @@X from demand mode
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo of affected run
//      cCommand:           true for @@X C - generates contingency information which the
//                              task (if any) can handle of it has registered to do so.
//      rCommand:           terminates anything currently executing, and removes any
//                              DCSE entries generated by the currently-executing program,
//                              including the one (if such exists) which invoked the program
//      tCommand:           terminates anything currently executing, removes all DCSE entries
//
//  Any combination of the above commands is acceptable.
void
Exec::abortRunDemand
(
    RunInfo* const      pRunInfo,
    const bool          cCommand,
    const bool          rCommand,
    const bool          tCommand
)
{
    pRunInfo->attach();

    //  Do contingency stuff?
    if ( cCommand )
    {
        //TODO:TASK
    }

    //  Terminate current task?
    //  TODO:TASK

    //  Terminate current command?
    ControlModeRunInfo* pcmri = dynamic_cast<ControlModeRunInfo*>( pRunInfo );
    if ( pcmri )
    {
        //  If there's a control mode activity, make it stop (but don't wait for it).
        Activity* pActivity = pcmri->getControlModeActivity();
        if ( pActivity )
            pActivity->stop( Activity::STOP_DEMAND_X, false );
    }

    //  Remove DCSE entries
    //  TODO:QECL

    abortRun( pRunInfo );
    pRunInfo->detach();
}


//  abortRunFacilities()
//
//  Aborts a run due to facilities request (incorrect read/write key given).
//  There is never any chance given to the current task (if any) to catch this contingency.
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo of affected run
void
Exec::abortRunFacilities
(
    RunInfo* const pRunInfo
)
{
    pRunInfo->attach();

    //  Is there a task active?  Terminate it with no chance of catching the contingency.
    //  Also, do run condition word stuff.  Don't allow PMD (only for batch).
    //  TODO:TASK

    //  General common abort handling
    abortRun( pRunInfo );
    pRunInfo->detach();
}


//  bootExec()
//
//  Boots the EXEC
bool
Exec::bootExec
(
    const bool              operatorBoot
)
{
    if ( (m_Status != ST_NOT_BOOTED) && (m_Status != ST_STOPPED) )
        return false;

    // Remove any existing RunInfo objects (from a previous session, presumably)
    m_pRunInfo = 0;
    while ( m_RunInfoTable.size() != 0 )
    {
        delete m_RunInfoTable.begin()->second;
        m_RunInfoTable.erase( m_RunInfoTable.begin()->first );
    }

    m_OperatorBoot = operatorBoot;
    return Worker::workerStart();
}


//  cleanupExec()
//
//  Application should call here just before deleting the Exec object.
//  We propogate the call to all the managers so they can clean up in preparation for deletion.
void
Exec::cleanupExec()
{
    for ( const auto& pmgr : m_Managers )
        pmgr->cleanup();
}


//  closeUserRun()
//
//  Closes a run.
//      Prints FIN message on console (if appropriate)
//      Prints tail sheet to output buffer (again, if appropriate)
//      Waits for output buffer to clear
//          don't hang here, if we are suddenly terminating, forget it.
//      If no files are queued from this RunInfo object, delete the object.
//
//  Parameters:
//      pActivity:          pointer to Exec activity which is invoking us
//      pRunInfo:           pointer to RunInfo which is to be closed
void
Exec::closeUserRun
(
    Activity* const         pActivity,
    UserRunInfo* const      pRunInfo
)
{
    if ( pRunInfo->hasLiveActivity() )
    {
        std::string logMsg = "Exec::closeUserRun():Called on RunInfo with live activity RunId=";
        logMsg += pRunInfo->getActualRunId();
        SystemLog::write( logMsg );
        stopExec( SC_INTERNAL_ERROR );
        return;
    }

    //  Emit {runid} FIN to console
    std::string consMsg = pRunInfo->getActualRunId();
    if ( pRunInfo->getStatus() != RunInfo::STATUS_NO_ERROR )
        consMsg += " " + RunInfo::getStatusString( pRunInfo->getStatus() );

    consMsg += " FIN";
    if ( m_pConfiguration->getBoolValue( "SFTIMESTAMP" ) )
        consMsg += " " + getTimeStamp();
    ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
    pcmgr->postReadOnlyMessage( consMsg, pRunInfo );

    //  If PRINT$ and/or PUNCH$ are breakpointed, clear that up
    //TODO:BATCH

    //  Print tailsheet to PRINT$
    if ( pRunInfo->isBatch() || pRunInfo->isDemand() )
        printTailSheet( dynamic_cast<ControlModeRunInfo*>(pRunInfo) );

    //  Release PRINT$ and/or PUNCH$ if appropriate, and queue them
    //TODO:BATCH

    //  Release all facilities (let FacilitiesManager deal with initial reserve release, cataloging, and dropping cycles)
    const FACITEMS& facItems = pRunInfo->getFacilityItems();
    FacilitiesManager* pfmgr = dynamic_cast<FacilitiesManager*>( m_Managers[MID_FACILITIES_MANAGER] );
    while ( !facItems.empty() )
    {
        pfmgr->releaseFacilityItemRunTermination( pActivity,
                                                  m_pRunInfo->getSecurityContext(),
                                                  pRunInfo,
                                                  facItems.begin()->second->getIdentifier(),
                                                  (pRunInfo->getStatus() != RunInfo::STATUS_NO_ERROR) );
    }

    //  Generally, we could just let the RunInfo object be discarded, and that would empty the
    //  NameItems container - since NameItem owns no dynamically-allocated memory, that's okay.
    //  However, we just don't want to be in the situation where NameItems might exist which think
    //  they are referencing FacItems, with all the FacItems gone.  So...
    pRunInfo->discardAllNameItems();
}


//  createDemandRun()
//
//  Opens a run for a demand session.
//  Sends output directly to the RSI session, since we don't yet have a RunInfo object
//  through which we can buffer our output.
//
//  Parameters:
//      pActivity:          pointer to requesting Activity
//      userProfile:        user's profile from SecurityManger
//      rsiSessionNumber:   RSI session number for the demand session
//      rsiSessionName:     RSI session name for the demand session
//      runId:              run-id from the @RUN image.  Must be a valid run-id if not empty
//                              if not provided, we start with 'RUN000'
//                              Eventually, we generate a unique run-id based on this value.
//                              If we have a unique run-id conflict, we complain to the RSI session and fail.
//      accountId:          account-id from the @RUN image.  Must be valid if not empty
//                              If not provided, we complain to the RSI session and fail
//                              If the user has no access to this account, we complain to the RSI session and fail.
//      userId:             user-id from the @RUN image.
//                              If provided, it must match the user-id for the RSI session
//      projectId:          project-id from the @RUN image.
//                              If not provided, it defaults to 'Q$Q$Q$'
//      options:            @RUN command options.  We verify that the given options are valid.
//      schedulingPriority: Priority in the print queue.  'A' through 'Z' - if blank, we assume 'A'.
//      processorDispatchingPriority:
//                          Determines this run's priority for scheduling processor time.
//                              'A' through 'Z' - if blank, we assume 'A'.
//                              Since we don't control this, we ignore this value.
//  Returns:
//      pointer to newly-created DemandRunInfo object if successful, else false
DemandRunInfo*
Exec::createDemandRun
(
    Activity* const                     pActivity,
    const SecurityManager::UserProfile& userProfile,
    const COUNT                         rsiSessionNumber,
    const std::string&                  rsiSessionName,
    const std::string&                  runId,
    const std::string&                  accountId,
    const std::string&                  userId,
    const std::string&                  projectId,
    const UINT32                        options,
    const char                          schedulingPriority,
    const char                          processorDispatchingPriority
)
{
    //  Validate account - we have to do this in case RSI screws things up
    RSIManager* prsimgr = dynamic_cast<RSIManager*>( m_Managers[MID_RSI_MANAGER] );
    ConsoleManager* pconsmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );

    if ( accountId.size() == 0 )
    {
        prsimgr->sendOutput( rsiSessionNumber, "*MISSING ACCOUNT*" );
        return 0;
    }

    //  Check user-id, if given
    SuperString sessionUserId;
    RSIManager::Status rsiStat = prsimgr->getUserId( rsiSessionNumber, &sessionUserId );
    if ( rsiStat != RSIManager::ST_SUCCESSFUL )
        return 0;

    if ( userId.size() > 0 )
    {
        if ( sessionUserId.compareNoCase( userId ) != 0 )
        {
            prsimgr->sendOutput( rsiSessionNumber, "USER-ID DIFFERENT FROM LOG-ON" );
            return 0;
        }
    }

    //  Make sure this user has access to the indicated account
    //  TODO:SEC

    //  Make sure we have access to the indicated project-id.
    //  "YOUR USER-ID DOES NOT HAVE ACCESS TO THE REQUESTED PROJECT-ID."
    //  TODO:SEC (? is this not handled in demand?  think about this...)

    //  Deal with project-id
    std::string effectiveProjectId = projectId;
    if ( effectiveProjectId.size() == 0 )
        effectiveProjectId = "Q$Q$Q$";

    //  Deal with run-id
    std::string originalRunId = runId;
    if ( originalRunId.size() == 0 )
        originalRunId = "RUN000";

    std::string actualRunId;
    if ( !generateUniqueRunId( runId, &actualRunId ) )
    {
        std::string consMsg = "UNABLE TO GENERATE UNIQUE RUNID FOR " + originalRunId;
        pconsmgr->postReadOnlyMessage( consMsg, m_pRunInfo );
        prsimgr->sendOutput( rsiSessionNumber, "RUN-ID CONFLICT" );
        return 0;
    }

    //  Priority values
    char schedPri = schedulingPriority;
    if ( schedPri == ' ' )
        schedPri = 'A';

    char procPri = processorDispatchingPriority;
    if ( procPri == ' ' )
        procPri = 'A';

    //  Ready to create and register the DemandRunInfo object
    DemandRunInfo* pDRInfo = new DemandRunInfo( this,
                                                originalRunId,
                                                actualRunId,
                                                accountId,
                                                projectId,
                                                sessionUserId,
                                                options,
                                                schedPri,
                                                procPri,
                                                rsiSessionNumber );
    pDRInfo->setSecurityContext( new SecurityContext( userProfile ) );
    m_RunInfoTable[actualRunId] = pDRInfo;

    //  Notify console
    std::string consMsg = actualRunId + "/" + rsiSessionName + " START";
    if ( m_pConfiguration->getBoolValue( "SFTIMESTAMP" ) )
        consMsg += " " + getTimeStamp();
    pconsmgr->postReadOnlyMessage( consMsg, pDRInfo );

    //  Assign TPF$ and DIAG$ - If either of these fails, the run is error'd and terminated.
    if ( !assignTPF$ToRun( pActivity, pDRInfo ) )
        return pDRInfo;
    if ( !assignDIAG$ToRun( pActivity, pDRInfo ) )
        return pDRInfo;

    return pDRInfo;
}


//  createKeyin()
//
//  Attempts to create a KeyinActivity based on a given unsolicited keyin, and to start it.
//
//  Parameters:
//      routing:        Console which is requesting this
//      text:           text from which a keyin is to be created
//
//  Returns:
//      true if an activity was created and started, else false (usually indicates an unrecognized keyin)
bool
Exec::createKeyin
(
    const Word36        routing,
    const std::string&  text
)
{
    KeyinActivity* pKeyin = KeyinActivity::createKeyin( this, text, routing );
    if ( !pKeyin )
    {
        ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( getManager( MID_CONSOLE_MANAGER ) );
        pConsMgr->postReadOnlyMessage( "KEYIN UNRECOGNIZED", routing, m_pRunInfo );
        return false;
    }

    registerAndStartKeyin( pKeyin );
    return true;
}


//  displaySystemErrorMsg()
//
//  Displays the
//      SYSTEM ERROR ooo TERMINATED SESSION ooo
//  diagnostic message
//
//  Parameters:
//      stopCode:           error cod to be displayed
//      session:            session number to be reported
void
Exec::displaySystemErrorMsg
(
    const StopCode          stopCode,
    const COUNT             session
) const
{
    std::stringstream strm;
    strm << "SYSTEM ERROR ";
    strm << std::oct << std::setw( 3 ) << std::setfill( '0' ) << static_cast<unsigned int>(stopCode);
    strm << " TERMINATED SESSION ";
    strm << std::oct << std::setw( 3 ) << std::setfill( '0' ) << session;

    ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
    pcmgr->postReadOnlyMessage( strm.str(), 0 );
}


//  displayTime()
//
//  Displays the current time on the console.
//  Shows long version after midnight.
//  Exec will call here every 6 minutes.
//
//  Parameters:
//      execTime:           exec system time of interest
//      longFormat:         true for long format, else false
//      routing:            console routing word
void
Exec::displayTime
(
    const EXECTIME          execTime,
    const bool              longFormat,
    const Word36&           routing
)
{
    std::stringstream strm;
    SystemTime* pSystemTime = SystemTime::createFromMicroseconds( SystemTime::getMicrosecondsSinceEpoch() );

    if ( longFormat )
    {
        // do long message
        strm << "Today is " << DayTable[pSystemTime->getDayOfWeek() - 1] << " ";
        strm << MonthTable[pSystemTime->getMonth() - 1] << " ";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getDay() << " ";
        strm << pSystemTime->getYear();
        strm << "  The time is ";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getHour() << ":";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getMinute() << ":";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getSecond();
    }
    else
    {
        // do short message
        strm << "                                        T/D  ";
        strm << pSystemTime->getYear() << "-";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getMonth() << "-";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getDay() << "  ";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getHour() << ":";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getMinute() << ":";
        strm << std::setw( 2 ) << std::setfill( '0' ) << pSystemTime->getSecond();
    }

    delete pSystemTime;
    pSystemTime = 0;

    ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
    pcmgr->postReadOnlyMessage( strm.str(), routing, m_pRunInfo );
}


//  dump()
//
//  Mostly for debugging
//
//  Returns:
//      file name of dump file
std::string
Exec::dump
(
    const UINT32            dumpBits
)
{
    SystemTime* pLocalTime = SystemTime::createLocalSystemTime();
    std::string fileName = "ExecDump." + pLocalTime->getTimeStamp();
#ifdef  WIN32
    fileName += ".txt";
#endif
    delete pLocalTime;
    pLocalTime = 0;

    std::ofstream stream( fileName );

    stream << "Exec Dump" << std::endl;
    stream << "  Current Session:   0" << std::oct << m_CurrentSession << std::endl;
    stream << "  Initial Boot:      " << ( m_InitialBoot ? "true" : "false" ) << std::endl;
    stream << "  Last Stop Code:    " << std::oct << static_cast<unsigned int>(m_LastStopCode)
            << ":" << getStopCodeString( m_LastStopCode ) << std::endl;
    stream << "  Operator Boot:     " << ( m_OperatorBoot ? "true" : "false" ) << std::endl;
    stream << "  Status:            " << std::oct << static_cast<unsigned int>(m_Status)
            << ":" << getStatusString( m_Status ) << std::endl;

    //  Dump the Run Info Table (this will include the Exec's table)
    stream << "  Run Info Objects:" << std::endl;
    for (CITRUNINFOS itri = m_RunInfoTable.begin(); itri != m_RunInfoTable.end(); ++itri )
        itri->second->dump( stream, "    ", dumpBits );

    //  Dump the managers
    for (CITEXECMANAGERS itm = m_Managers.begin(); itm != m_Managers.end(); ++itm )
        (*itm)->dump( stream, dumpBits );

    stream.close();
    return fileName;
}


//  getExecTime()
//
//  Gets the exec time in microseconds since EXEC epoch,
//  based on the offset provided by the D keyin.
EXECTIME
Exec::getExecTime() const
{
    return SystemTime::getMicrosecondsSinceEpoch() + m_ExecTimeOffsetMicros;
}


//  getExecTimeTDate()
//
//  Gets Exec time in TDATE$ form
void
Exec::getExecTimeTDate
(
TDate* const        pTDate
) const
{
    EXECTIME execTime = getExecTime();
    SystemTime* pSystemTime = SystemTime::createFromMicroseconds( execTime );
    COUNT32 seconds = pSystemTime->getHour() * 3600 + pSystemTime->getMinute() * 60 + pSystemTime->getSecond();
    pTDate->setMonth( pSystemTime->getMonth() );
    pTDate->setDay( pSystemTime->getDay() );
    pTDate->setYear( pSystemTime->getYear() - 1961 );
    pTDate->setSeconds( seconds );
}


//  getJumpKeys()
//
//  Retrieves jump keys for callers as a vector where the vector index is one less than the jump key id.
void
Exec::getJumpKeys
(
    std::vector<bool>* const    pJumpKeys
)
{
    pJumpKeys->resize(36);

    UINT64 mask = 01L;
    UINT64 value = m_pPanelInterface->getJumpKeys().getValue();
    PanelInterface::JUMPKEY jumpKey = 36;
    while ( jumpKey > 0 )
    {
        (*pJumpKeys)[jumpKey - 1] = (mask & value) != 0;
        mask <<= 1;
        --jumpKey;
    }
}


//  getRunCounters()
//
//  Used by SS keyin to display status information
void
Exec::getRunCounters
(
    COUNT* const        pBatchRunCount,
    COUNT* const        pDemandRunCount,
    COUNT* const        pBacklogRunCount
) const
{
    *pBatchRunCount = 0;
    *pDemandRunCount = 0;
    *pBacklogRunCount = 0;

    lock();
    for ( CITRUNINFOS itri = m_RunInfoTable.begin(); itri != m_RunInfoTable.end(); ++itri )
    {
        const RunInfo* pRunInfo = itri->second;
        switch ( pRunInfo->getState() )
        {
        case RunInfo::STATE_IN_BACKLOG:
            ++(*pBacklogRunCount);
            break;

        case RunInfo::STATE_ACTIVE:
        case RunInfo::STATE_FIN:
        case RunInfo::STATE_RSI_DETACHED:
        case RunInfo::STATE_RSI_TERM:
            if ( pRunInfo->isBatch() )
                ++(*pBatchRunCount);
            else if ( pRunInfo->isDemand() )
                ++(*pDemandRunCount);
            break;

        case RunInfo::STATE_SMOQUE:
            //???? anything to do here?
            break;
        }
    }
    unlock();
}


//  getRunInfo()
//
//  Retrieves the RunInfo object for the given actual run id,
//  possibly attaching it in the process.
RunInfo*
    Exec::getRunInfo
    (
    const std::string&  runId,
    const bool          attachFlag
    ) const
{
    RunInfo* pResult = 0;

    lock();
    CITRUNINFOS itri = m_RunInfoTable.find( runId );
    if ( itri != m_RunInfoTable.end() )
    {
        pResult = itri->second;
        if ( attachFlag )
            pResult->attach();
    }

    unlock();
    return pResult;
}


//  getRunids()
//
//  Populates a container of strings with the Runid's of all the RunInfo objects in our table
void
    Exec::getRunids
    (
    LSTRING* const      pContainer
    ) const
{
    pContainer->clear();
    lock();
    for ( CITRUNINFOS itri = m_RunInfoTable.begin(); itri != m_RunInfoTable.end(); ++itri )
        pContainer->push_back( itri->first );
    unlock();
}


//  getStatusCounters()
//
//  Used by SS keyin to display status information
void
Exec::getStatusCounters
(
    COUNT* const pRunsWaitingOnStartTime,
    COUNT* const pUnopenedDeadlineRuns,
    COUNT* const pOperationsHold,
    COUNT* const pSOptionHold,
    COUNT* const pActiveRuns,
    COUNT* const pInitialLoadRuns
) const
{
    *pRunsWaitingOnStartTime = 0;
    *pUnopenedDeadlineRuns = 0;
    *pOperationsHold = 0;
    *pSOptionHold = 0;
    *pActiveRuns = 0;
    *pInitialLoadRuns = 0;

    lock();
    for ( CITRUNINFOS itri = m_RunInfoTable.begin(); itri != m_RunInfoTable.end(); ++itri )
    {
        const RunInfo* pRunInfo = itri->second;
        if ( !pRunInfo->isExec() && !pRunInfo->isTIP() )
        {
            switch ( pRunInfo->getState() )
            {
            case RunInfo::STATE_IN_BACKLOG:
                //TODO:BATCH any deadline?  any waiting on start time?  any ops hold?
                break;

            case RunInfo::STATE_ACTIVE:
                ++(*pActiveRuns);
                //TODO:TASK any in initial load?
                break;

            case RunInfo::STATE_FIN:
            case RunInfo::STATE_RSI_DETACHED:
            case RunInfo::STATE_RSI_TERM:
                //  We don't count these
                break;

            case RunInfo::STATE_SMOQUE:
                //???? anything to do here?
                break;
            }
        }
    }
    unlock();
}


//  pollUserRunInfoObjects()
//
//  Iterates over the user-mode RunInfo objects, and cleans their terminated activities.
void
Exec::pollUserRunInfoObjects
(
    Activity* const     pActivity
)
{
    lock();

    ITRUNINFOS itri = m_RunInfoTable.begin();
    while ( itri != m_RunInfoTable.end() )
    {
        RunInfo* pri = itri->second;
        UserRunInfo* prui = dynamic_cast<UserRunInfo*>( pri );
        pri->attach();
        switch ( pri->getState() )
        {
        case RunInfo::STATE_IN_BACKLOG:
            //  Waiting on QueueManager to start us
            pri->detach();
            ++itri;
            break;

        case RunInfo::STATE_ACTIVE:
            //  Clean up any stale activity threads
            pri->cleanActivities();
            pri->detach();
            ++itri;
            break;

        case RunInfo::STATE_FIN:
            //  Clear the run info.
            closeUserRun( pActivity, prui );

            //  If we're DEMAND, wait for RSIManager to let go of us
            //  otherwise, go to ST_SMOQUE
            if ( pri->isDemand() && (pri->getState() != RunInfo::STATE_RSI_DETACHED) )
                pri->setState( RunInfo::STATE_RSI_TERM );
            else
                pri->setState( RunInfo::STATE_SMOQUE );
            pri->detach();
            break;

        case RunInfo::STATE_RSI_DETACHED:
            //  RSIManager no longer tracking us - move on to ST_SMOQUE
            pri->setState( RunInfo::STATE_SMOQUE );
            pri->detach();
            break;

        case RunInfo::STATE_RSI_TERM:
            //  Waiting on RSIManager to release us
            pri->detach();
            ++itri;
            break;

        case RunInfo::STATE_SMOQUE:
            //  There (may) be files in SMOQUE - if not, we can delete the RunInfo object
            //TODO:SMOQUE
            if ( false ) // in smoque
            {
                pri->detach();
                ++itri;
            }

            //  Not in smoque...
            pri->detach();
            pri = 0;
            delete itri->second;
            itri->second = 0;
            m_RunInfoTable.erase( itri++ );
            break;
        }
    }

    unlock();
}


//  postNodeChangeEvent()
//
//  For managers (i.e., DeviceManager and MFDManager) which become aware of changes to some Node
//  which is known only to them (that is to say, NOT known to the Node itself).
//???? obsolete for now, but we may need it again later
/*
void
    Exec::postNodeChangeEvent
    (
    const Node* const       pNode
    )
{
    //  Call into DeviceManager to get the identifier of the node
    DeviceManager* pDevMgr = dynamic_cast<DeviceManager*>( getManager( MID_DEVICE_MANAGER ) );
    DeviceManager::NODE_ID nodeId = pDevMgr->getNodeIdentifier( pNode );

    //  Update status string for all nodes
    const DeviceManager::NodeEntry* pNodeEntry = pDevMgr->getNodeEntry( nodeId );
    std::string nodeStatusString = DeviceManager::getNodeStatusString( pNodeEntry->m_Status );

    //  Update mounted media name for disk and tape only
    std::string mediaName;
    if ( pNode->getCategory() == Node::Category::DEVICE )
    {
        const Device* pDevice = reinterpret_cast<const Device*>( pNode );
        switch ( pDevice->getDeviceType() )
        {
            case Device::DeviceType::DISK:
            {
                MFDManager* pMfdMgr = dynamic_cast<MFDManager*>( getManager( MID_MFD_MANAGER ) );
                const MFDManager::PackInfo* pPackInfo = pMfdMgr->getPackInfo( nodeId );
                if ( pPackInfo )
                    mediaName = pPackInfo->m_PackName;
                break;
            }
            case Device::DeviceType::SYMBIONT:
            {
                //TODO:SYMB
                break;
            }
            case Device::DeviceType::TAPE:
            {
                //TODO:TAPE
                break;
            }
        }
    }

    ExecNodeChangeEvent ev( pNode, mediaName, nodeStatusString );
    notifyListeners( &ev );
}
*/


//  printTailSheet()
//
//  Prints summary information to a run's tail sheet.
//  Should only call us for batch and demand (not for TIP)
//
//  Format:
//      RUNID: XXXXXX     ACCT: XXXXXXXXXXXX   PROJECT: XXXXXXXXXXXX
//      SITE:  XXXXXX   USERID: XXXXXXXXXXXX   SYSTEM:  XXXXXXXXXXXX
//      CONSOLE MESSAGE TEXT LOCATION
//      DEVICE USAGE: REQUESTS TRANSFERRED
//      DISK:                0           0 WORDS
//      TAPE:                0           0 WORDS
//      MAX TAPES ASG: 0
//      RESOURCE WAIT: 00:00:00.000
//      MEMORY USAGE: 0 PAGE�SECONDS
//      MAX MEMORY: 0 PAGES
//      TIME: TOTAL: 00:00:00.000
//      CPU: 00:00:00.000 I/O: 00:00:00.000
//      CC/ER: 00:00:00.000 WAIT: 00:00:00.000
//      SUAS USED: 0.00 SUAS REMAINING: 0.00
//      IMAGES READ: 0 PAGES: 0
//      START: 00:00:00 MON dd,yyyy FIN: 00:00:00 MON dd,yyyy
void
Exec::printTailSheet
(
    ControlModeRunInfo* const   pRunInfo
) const
{
    //  Setting up stuff...
    SuperString ssRunId = pRunInfo->getActualRunId();
    SuperString ssAccount = pRunInfo->getAccountId();
    SuperString ssProject = pRunInfo->getProjectId();
    SuperString ssSite("SITEID");   //TODO:CONFIG Need to get SITE-ID from Configurator?
    SuperString ssUserId = pRunInfo->getUserId();
    SuperString ssSystem("SYSID");  //TODO:CONFIG System ID from where?

    ssRunId.resize( 6, ' ' );
    ssAccount.resize( 12, ' ' );
    ssProject.resize( 12, ' ' );
    ssSite.resize( 6, ' ' );
    ssUserId.resize( 12, ' ' );
    ssSystem.resize( 12, ' ' );

    const RunInfo::Counters& counters = pRunInfo->getCounters();

    EXECTIME nowMicros = getExecTime();
    COUNT64 totalTimeMillisTotal = (nowMicros - pRunInfo->getStartTime()) / 1000;
    COUNT totalTimeMillis = totalTimeMillisTotal / 1000;
    COUNT totalTimeSeconds = (totalTimeMillisTotal / 1000) % 60;
    COUNT totalTimeMinutes = (totalTimeMillisTotal / 60000) % 60;
    COUNT totalTimeHours = static_cast<COUNT>( totalTimeMillisTotal / 360000 );

    //  Printing
    //      RUNID: XXXXXX     ACCT: XXXXXXXXXXXX   PROJECT: XXXXXXXXXXXX
    std::stringstream strm;
    strm << "RUNID: " << ssRunId << "     ACCT: " << ssAccount << "   PROJECT: " << ssProject;
    pRunInfo->postToPrint( strm.str() );

    //      SITE:  XXXXXX   USERID: XXXXXXXXXXXX   SYSTEM:  XXXXXXXXXXXX
    strm.str("");
    strm << "SITE:  " << ssSite << "   USERID: " << ssUserId << "   SYSTEM:  " << ssSystem;
    pRunInfo->postToPrint( strm.str() );

    //      CONSOLE MESSAGE TEXT LOCATION
    const LSTRING consLog = pRunInfo->getConsoleLog();
    for ( LCITSTRING its = consLog.begin(); its != consLog.end(); ++its )
    {
        strm.str("");
        strm << "   " << *its;
        pRunInfo->postToPrint( strm.str() );
    }

    //      DEVICE USAGE: REQUESTS TRANSFERRED
    //      DISK:                0           0 WORDS
    //      TAPE:                0           0 WORDS
    pRunInfo->postToPrint( "DEVICE USAGE: REQUESTS TRANSFERRED" );
    strm.str("");
    strm << "DISK:         "
        << std::dec << std::setw( 8 ) << std::setfill( ' ' ) << counters.m_DiskRequestCount
        << " " << std::dec << std::setw( 11 ) << std::setfill( ' ' ) << counters.m_DiskWordCount << " WORDS";
    pRunInfo->postToPrint( strm.str() );
    strm.str("");
    strm << "TAPE:         "
        << std::dec << std::setw( 8 ) << std::setfill( ' ' ) << counters.m_TapeRequestCount
        << " " << std::dec << std::setw( 11 ) << std::setfill( ' ' ) << counters.m_TapeWordCount << " WORDS";
    pRunInfo->postToPrint( strm.str() );

    //      MAX TAPES ASG: 0
    strm.str("");
    //TODO:TAPE
    strm << "MAX TAPES ASG: 0";
    pRunInfo->postToPrint( strm.str() );

    //      RESOURCE WAIT: 00:00:00.000
    const RunInfo::Counters& riCounters = pRunInfo->getCounters();
    strm.str("");
    COUNT64 working = riCounters.m_ResourceWaitMilliseconds;
    COUNT64 rwMillis = working % 1000;
    working /= 1000;
    COUNT64 rwSeconds = working % 60;
    working /= 60;
    COUNT64 rwMinutes = working % 60;
    COUNT64 rwHours = working / 60;
    strm << "RESOURCE WAIT: "
        << std::setw( 2 ) << std::setfill( '0' ) << rwHours
        << ":" << std::setw( 2 ) << std::setfill( '0' ) << rwMinutes
        << ":" << std::setw( 2 ) << std::setfill( '0' ) << rwSeconds
        << "." << std::setw( 3 ) << std::setfill( '0' ) << rwMillis;
    pRunInfo->postToPrint( strm.str() );

    //      MEMORY USAGE: 0 PAGE�SECONDS
    strm.str("");
    //TODO:TASK
    strm << "MEMORY USAGE: 0 PAGE-SECONDS";
    pRunInfo->postToPrint( strm.str() );

    //      MAX MEMORY: 0 PAGES
    strm.str("");
    //TODO:TASK
    strm << "MAX MEMORY: 0 PAGES";
    pRunInfo->postToPrint( strm.str() );

    //      TIME: TOTAL: 00:00:00.000
    strm.str("");
    strm << "TIME: TOTAL: ";
    strm << std::setw( 2 ) << std::setfill( '0' ) << totalTimeHours << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << totalTimeMinutes << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << totalTimeSeconds << ".";
    strm << std::setw( 3 ) << std::setfill( '0' ) << totalTimeMillis;
    pRunInfo->postToPrint( strm.str() );

    //      CPU: 00:00:00.000 I/O: 00:00:00.000
    strm.str("");
    //TODO:TASK
    strm << "CPU: 00:00:00.000 I/O: 00:00:00.000";
    pRunInfo->postToPrint( strm.str() );

    //      CC/ER: 00:00:00.000 WAIT: 00:00:00.000
    strm.str("");
    //TODO:TASK
    strm << "CC/ER: 00:00:00.000 WAIT: 00:00:00.000";
    pRunInfo->postToPrint( strm.str() );

    //      SUAS USED: 0.00 SUAS REMAINING: 0.00
    strm.str("");
    //TODO:TASK
    strm << "SUAS USED: 0.00 SUAS REMAINING: 0.00";
    pRunInfo->postToPrint( strm.str() );

    //      IMAGES READ: 0 PAGES: 0
    strm.str("");
    strm << "IMAGES READ: " << counters.m_ImagesRead << " PAGES: " << counters.m_PagesPrinted;
    pRunInfo->postToPrint( strm.str() );

    //      START: 00:00:00 MON dd,yyyy FIN: 00:00:00 MON dd,yyyy
    SystemTime* pStartTime = SystemTime::createFromMicroseconds( pRunInfo->getStartTime() );
    SystemTime* pEndTime = SystemTime::createFromMicroseconds( nowMicros );

    strm.str("");
    strm << "START: ";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getHour() << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getMinute() << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getSecond() << " ";
    strm << MonthTable[pStartTime->getMonth() - 1] << " ";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getDay() << ",";
    strm << std::setw( 4 ) << std::setfill( '0' ) << pStartTime->getYear() << " ";
    strm << "FIN: ";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pEndTime->getHour() << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pEndTime->getMinute() << ":";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pEndTime->getSecond() << " ";
    strm << MonthTable[pEndTime->getMonth() - 1] << " ";
    strm << std::setw( 2 ) << std::setfill( '0' ) << pEndTime->getDay() << ",";
    strm << std::setw( 4 ) << std::setfill( '0' ) << pEndTime->getYear() << " ";
    pRunInfo->postToPrint( strm.str() );

    delete pStartTime;
    pStartTime = 0;
    delete pEndTime;
    pEndTime = 0;

    return;
}


//  registerAndStartKeyin()
//
//  An unstarted KeyinActivity is passed to us.
//  We add it to our local task's activity list, and start it.
//
//  Called by ConsoleActivity
void
Exec::registerAndStartKeyin
(
    KeyinActivity* const    pActivity
)
{
    lock();
    m_pRunInfo->appendTaskActivity( pActivity );
    pActivity->start();
    unlock();
}


//  setStatus()
//
//  All updates to the Exec status MUST go through here.
//  For one reason, we need to notify the Application object every time the state changes.
//  There may emerge other reasons.
//
//  Parameters:
//      status:                 new status
void
Exec::setStatus
(
    const Status                status
)
{
    //  Update status, then notify listeners
    m_Status = status;
    m_pPanelInterface->setStatusMessage( getStatusString( m_Status ) );
}


//  stopExec()
//
//  Stops the EXEC
bool
Exec::stopExec
(
    const StopCode              stopCode
)
{
    if ( !isRunning() )
        return false;

    m_LastStopCode = stopCode;
    m_pPanelInterface->setStopCodeMessage( getStopCodeString( m_LastStopCode ) );

    Worker::workerStop( false );
    return true;
}


//  terminateDemandRun()
//
//  Called whenever the RSI session is abruptly terminated.
//  Start the process for cleaning up the run.
void
Exec::terminateDemandRun
(
    DemandRunInfo* const    pRunInfo
)
{
    //  Handle any breakpointed symbiont buffers (close them, queue them, whatever),
    //  and lose the RSISymbiontBuffer (session is no longer available).
    SymbiontBuffer* psbPrint = pRunInfo->getSymbiontBufferPrint();
    while ( psbPrint )
    {
        if ( psbPrint->getBufferType() != SymbiontBuffer::BUFTYP_RSI )
        {
            //TODO:BRKPT
        }

        pRunInfo->popSymbiontBufferPrint();
        psbPrint = pRunInfo->getSymbiontBufferPrint();
    }

    //  If there is a control mode activity, abort it and set FIN status.
    //  Eventually CoarseScheduler will find the activity terminated, clear it, and call closeRun()
    Activity* pcmAct = pRunInfo->getControlModeActivity();
    if ( pcmAct && !pcmAct->isTerminated() )
    {
        pcmAct->stop( Activity::STOP_DEMAND_TERM, false );
        pRunInfo->setState( RunInfo::STATE_FIN );
        return;
    }

    //  If there is a task, abort it and set FIN status.
    //  Eventually CoarseScheduler will find the activity terminated, clear it, and call closeRun()
    //  TODO:TASK

    //  No task or activity, user is just sitting there in control mode.
    //  Go ahead and ABORT the run, set FIN status, and close it.
    std::string consMsg = pRunInfo->getActualRunId() + " ABORT";
    ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_Managers[MID_CONSOLE_MANAGER] );
    pcmgr->postReadOnlyMessage( consMsg, m_pRunInfo );

    RunConditionWord& rcw = pRunInfo->getRunConditionWord();
    rcw.clearMostRecentActivityErrored();
    rcw.setMostRecentActivityAborted();
    if ( rcw.getPreviousTaskError() )
        rcw.setPreviousPreviousTaskError();
    rcw.setPreviousTaskError();

    pRunInfo->attach();
    pRunInfo->setState( RunInfo::STATE_FIN );
    pRunInfo->detach();
}



//  public statics

//  getSatusString()
const char*
Exec::getStatusString
(
const Status        status
)
{
    static char defaultMsg[16];

    switch ( status )
    {
    case ST_INIT:                   return "Initializing";
    case ST_BOOTING_1:              return "Booting stage 1";
    case ST_BOOTING_2:              return "Booting stage 2";
    case ST_NOT_BOOTED:             return "Not booted";
    case ST_RUNNING:                return "Running";
    case ST_STOPPED:                return "Stopped";
    case ST_TERMINATING:            return "Terminating";
    case ST_SYS:                    return "SYS running";
    }

    std::stringstream strm;
    UINT32 iStat = static_cast<UINT32>(status);
    if ( iStat > 0 ) {
        strm << "0";
    }
    strm << std::oct << iStat;
    strcpy( defaultMsg, strm.str().c_str() );
    return defaultMsg;
}


//  getStopCodeString()
const char*
Exec::getStopCodeString
(
    const StopCode      stopCode
)
{
    static char defaultMsg[16];

    switch ( stopCode )
    {
    case SC_NONE:                           return "No error";
    case SC_ARRAY_SUBSCRIPT_OUT_OF_RANGE:   return "Array subscript out of range";
    case SC_BOOT_TAPE_IO_ERROR:             return "Boot tape IO error";
    case SC_DEBUG:                          return "Debug stop";
    case SC_DIRECTORY_ERROR:                return "Directory error";
    case SC_DYNAMIC_ALLOCATOR:              return "Dynamic allocator failure";
    case SC_EQUIPMENT_CODE_NOT_CONFIGURED:  return "Equipment code not configured";
    case SC_EXEC_IN_CONTINGENCY:            return "EXEC in contingency";
    case SC_EXEC_IO_FAILED:                 return "EXEC IO failed";
    case SC_EXEC_USE_FAILED:                return "EXEC @use failed";
    case SC_FACILITIES_ERROR:               return "Facilities error or Response requires reboot";
    case SC_GENERIC_063:                    return "Generic stop";
    case SC_GENF_QUEUEING_PROBLEM:          return "GENF$ queueing problem";
    case SC_ILLOGICAL_DAYCLOCK:             return "Illogical dayclock";
    case SC_INITIALIZATION:                 return "Error during system initialization";
    case SC_INITIALIZATION_ASG_FAILED:      return "Initialization @asg failed";
    case SC_INTERNAL_ERROR:                 return "Internal error";
    case SC_INVALID_ADDRESS:                return "Invalid address";
    case SC_INVALID_LDAT_ADDRESS:           return "Invalid LDAT address";
    case SC_MASS_STORAGE_DEBUG_STOP:        return "Mass Storage debug stop";
    case SC_MASS_STORAGE_FULL:              return "Mass Storage full";
    case SC_NO_DEVICE_CONFIGURED:           return "No device configured";
    case SC_OPERATOR_KEYIN:                 return "Operator keyin or other fatal response";
    case SC_PANEL_HALT:                     return "System Control Processor or Panel requested IP halt";
    case SC_PATH_NOT_CONFIGURED:            return "Path not configured";
//      Following is same code as SC_FACILITIES_ERROR
//    case SC_RESPONSE_REQUIRES_REBOOT:       return "Response requires reboot";
    case SC_RUNID_FAILURE:                  return "RUNID cycle wrap - cannot generate unique Run Id";
    case SC_SECTOR_NOT_ALLOCATED:           return "Sector not allocated";
    case SC_START_ERROR:                    return "Could not start a manager";
    case SC_SYMBIONT_NAME_NOT_FOUND:        return "Symbiont name not found";
    case SC_TRACK_COUNT_MISMATCH:           return "Track count mismatch";
    case SC_TRACK_NOT_ALLOCATED:            return "Track not allocated";
    default:
        //  Avoid warnings wrt duplicate values
        break;
    }

    unsigned int iCode = static_cast<unsigned int>( stopCode );
    std::stringstream strm;
    if ( iCode > 0 ) {
        strm << "0";
    }
    strm << std::oct << iCode;
    strcpy( defaultMsg, strm.str().c_str() );
    return defaultMsg;
}


//  isValidAccountId()
//
//  1 to 12 characters, any combination of letters, numbers, '-' and '.'
bool
Exec::isValidAccountId
(
    const std::string&      accountId
)
{
    INDEX ac = accountId.size();
    if ( (ac < 1) || (ac > 12) )
        return false;
    for ( INDEX ax = 0; ax < ac; ++ax )
    {
        char ch = accountId[ax];
        if ( !isalpha( ch ) && !isdigit( ch ) && (ch != '-') && (ch != '.') )
            return false;
    }
    return true;
}


//  isValidFilename()
//
//  1 to 12 characters, any combination of letters, numbers, '-' and '$'
bool
Exec::isValidFilename
(
    const std::string&      filename
)
{
    INDEX ac = filename.size();
    if ( (ac < 1) || (ac > 12) )
        return false;
    for ( INDEX ax = 0; ax < ac; ++ax )
    {
        char ch = filename[ax];
        if ( !isalpha( ch ) && !isdigit( ch ) && (ch != '-') && (ch != '$') )
            return false;
    }
    return true;
}


//  isValidProjectId()
//
//  1 to 12 characters, any combination of letters, numbers, '-' and '$'.
bool
Exec::isValidProjectId
(
    const std::string&          projectId
)
{
    INDEX pc = projectId.size();
    if ( (pc < 1) || (pc > 12) )
        return false;
    for ( INDEX px = 0; px < pc; ++px )
    {
        char ch = projectId[px];
        if ( !isalpha( ch ) && !isdigit( ch ) && (ch != '-') && (ch != '$') )
            return false;
    }
    return true;
}


//  isValidQualifier()
//
//  1 to 12 characters, any combination of letters, numbers, '-' and '$'
bool
Exec::isValidQualifier
(
    const std::string&      qualifier
)
{
    INDEX ac = qualifier.size();
    if ( (ac < 1) || (ac > 12) )
        return false;
    for ( INDEX ax = 0; ax < ac; ++ax )
    {
        char ch = qualifier[ax];
        if ( !isalpha( ch ) && !isdigit( ch ) && (ch != '-') && (ch != '$') )
            return false;
    }
    return true;
}


//  isValidRunId()
//
//  1 to 6 characters, any combination of letters and numbers.
bool
Exec::isValidRunId
(
    const std::string&          runId
)
{
    INDEX rc = runId.size();
    if ( (rc < 1) || (rc > 12) )
        return false;
    for ( INDEX rx = 0; rx < rc; ++rx )
    {
        char ch = runId[rx];
        if ( !isalpha( ch ) && !isdigit( ch ) )
            return false;
    }
    return true;
}


//  isValidRWKey()
//
//  1 to 6 characters, selected from:
//      ! " # $ % & ' ( ) * + - 0:9 : < = > ? @ A:Z [ \ ] ^ _
//  Note: empty string is flagged as invalid.
bool
Exec::isValidRWKey
(
    const std::string&          key
)
{
    INDEX ks = key.size();
    if ( (ks < 1) || (ks > 6) )
        return false;

    for (INDEX kx = 0; kx < ks; ++kx)
    {
        if ( !ValidKeyCharacters[static_cast<unsigned int>(key[kx])] )
            return false;
    }

    return true;
}


//  isValidUserId()
//
//  1 to 12 characters, any combination of letters, numbers, '-' and '.'.
bool
Exec::isValidUserId
(
    const std::string&          projectId
)
{
    INDEX pc = projectId.size();
    if ( (pc < 1) || (pc > 12) )
        return false;
    for ( INDEX px = 0; px < pc; ++px )
    {
        char ch = projectId[px];
        if ( !isalpha( ch ) && !isdigit( ch ) && (ch != '-') && (ch != '.') )
            return false;
    }
    return true;
}


