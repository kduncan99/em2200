//	Exec class declaration



#ifndef		EXECLIB_EXEC_H
#define		EXECLIB_EXEC_H



#include    "Configuration.h"
#include    "ConsoleInterface.h"
#include    "ControlModeRunInfo.h"
#include    "DemandRunInfo.h"
#include    "ExecManager.h"
#include    "ExecRunInfo.h"
#include    "KeyinActivity.h"
#include    "NodeTable.h"
#include    "RunInfo.h"
#include    "UserRunInfo.h"
#include    "PanelInterface.h"



//  For debugging
#if 0 //TODO:DEBUG keep this around if we need it for some other similar purpose
#define     STRINGIZE(x)                    STRINGIZE_SIMPLE(x)
#define     STRINGIZE_SIMPLE(x)             #x

#define     ASSERT_OR_DEBUG_STOP(test)      ASSERT_OR_STOP((test), Exec::SC_DEBUG)

#ifdef _DEBUG
#define     ASSERT_OR_STOP(test, code)                                                              \
{                                                                                                   \
    if ( !(test) )                                                                                  \
    {                                                                                               \
        std::string str = "assert(" #test ") failed in " __FILE__ " at line " STRINGIZE(__LINE__);  \
        SystemLog::getInstance()->write( str );                                                     \
    }                                                                                               \
    assert(test);                                                                                   \
}
#else
#define     ASSERT_OR_STOP(test)                                                                    \
{                                                                                                   \
    if ( !(test) )                                                                                  \
    {                                                                                               \
        std::string str = "assert(" #test ") failed in " __FILE__ " at line " STRINGIZE(__LINE__);  \
        SystemLog::getInstance()->write( str );                                                     \
        Exec::getInstance()->stopExec( code );                                                      \
    }                                                                                               \
}
#endif
#endif



class	Exec : public Worker, public Lockable
{
public:
    enum ManagerId
    {
        //  Preserve this order; it will dictate the (reverse) order of shutting down which
        //  *might* be significant.
        MID_DEVICE_MANAGER,
        MID_IO_MANAGER,
        MID_CONSOLE_MANAGER,
        MID_MFD_MANAGER,
        MID_FACILITIES_MANAGER,
        MID_QUEUE_MANAGER,
        MID_ACCOUNT_MANAGER,
        MID_SECURITY_MANAGER,
        MID_RSI_MANAGER,
    };

private:
    static const COUNT              m_ManagerCount = 9;

public:
    enum Status
    {
        ST_NOT_BOOTED,
        ST_INIT,            //  just starting things up
        ST_BOOTING_1,       //  waiting for user to update configuration keys - allow $! D CJ DJ SJ UP DN SU RV FS
        ST_BOOTING_2,       //  Initializing / Recovering storage - allow D DJ FS
        ST_SYS,             //  Spans SYS START to SYS FIN - allow D DJ FS
        ST_RUNNING,         //  Allow remainder of keyins
        ST_TERMINATING,     //  Exec is in the process of shutting down
        ST_STOPPED,
    };

    enum StopCode
    {
        //  See EXEC System Console Messages Appendix A
        SC_NONE                             = 0,
        SC_FACILITIES_INVENTORY             = 001,  //  Something wrong with a facilities structure
        SC_ARRAY_SUBSCRIPT_OUT_OF_RANGE     = 006,  //  Any code that detects this problem, can use this
        SC_DYNAMIC_ALLOCATOR                = 010,  //  malloc() failed - if we do malloc anywhere
        SC_ILLOGICAL_DAYCLOCK               = 014,  //  something wrong processing the dayclock
        SC_GENF_QUEUEING_PROBLEM            = 023,  //  problems with GENF queueing mechanism
        SC_SYMBIONT_NAME_NOT_FOUND          = 024,  //  something wrong with symbiont configuration
        SC_EXEC_USE_FAILED                  = 031,  //  @USE for EXEC PCT failed
        SC_INITIALIZATION_ASG_FAILED        = 034,  //  Exec @ASG failed during boot
        SC_EXEC_IO_FAILED                   = 040,  //  Exec needed to do an IO, and it didn't work
        SC_RUNID_FAILURE                    = 044,  //  Full cycle reached generating RUNid's
        SC_EQUIPMENT_CODE_NOT_CONFIGURED    = 050,  //  Equipment code in MFD not configured
        SC_MASS_STORAGE_FULL                = 052,  //  EXEC request for MS cannot be satisfied
        SC_FACILITIES_ERROR                 = 055,  //  including DN of vital system component
        SC_RESPONSE_REQUIRES_REBOOT         = 055,  //  response to a console message requires a reboot
        SC_TRACK_COUNT_MISMATCH             = 056,  //  track count mismatch detected
        SC_TRACK_NOT_ALLOCATED              = 057,  //  track to be released was not allocated
        SC_GENERIC_063                      = 063,  //  Generic error
        SC_INITIALIZATION                   = 064,  //  Error detected in system configuration
        SC_DEBUG                            = 0101, //  something we do only for development debugging (also for MCT config error)
        SC_EXEC_IN_CONTINGENCY              = 0105, //  Something an Exec activity did, resulted in a contingency
        SC_BOOT_TAPE_IO_ERROR               = 0145, //  Fatal IO error reading boot tape
        SC_OPERATOR_KEYIN                   = 0150, //  or any fatal response
        SC_DIRECTORY_ERROR                  = 0151, //  Error dealing with MFD structures/links
        SC_SECTOR_NOT_ALLOCATED             = 0157, //  Sector to be released was not allocated, or is DAS0 or MFD sector 1
        SC_INVALID_ADDRESS                  = 0220, //  On IO
        SC_PATH_NOT_CONFIGURED              = 0221, //  On IO
        SC_NO_DEVICE_CONFIGURED             = 0222, //  On subchannel address for IO
        SC_INVALID_LDAT_ADDRESS             = 0253,
        SC_MASS_STORAGE_DEBUG_STOP          = 0266,
        SC_INTERNAL_ERROR                   = 0770, //  Some general coding exception we discovered (do this instead of assert())
        SC_PANEL_HALT                       = 0776, //  should probably be 0150.  or maybe not.  whatever.
        SC_START_ERROR                      = 0777,
    };


private:
    typedef std::vector<ExecManager*>                           EXECMANAGERS;
    typedef EXECMANAGERS::iterator                              ITEXECMANAGERS;
    typedef EXECMANAGERS::const_iterator                        CITEXECMANAGERS;

    typedef std::map<std::string, RunInfo*>                     RUNINFOS;
    typedef RUNINFOS::iterator                                  ITRUNINFOS;
    typedef RUNINFOS::const_iterator                            CITRUNINFOS;

    //  private data
    Configuration* const            m_pConfiguration;           //  From application wrapper (e.g., emexec)
    COUNT                           m_CurrentSession;
    INT64                           m_ExecTimeOffsetMicros;     //  usec offset from system time, for Exec time
    bool                            m_HoldBatchRuns;            //  Do not allow any more batch jobs out of backlog
    bool                            m_HoldDemandRuns;           //  Stop demand control statement scheduling
    bool                            m_HoldDemandTerminals;      //  Do not allow any more demand terminals to go active
    bool                            m_InitialBoot;
    StopCode                        m_LastStopCode;
    EXECMANAGERS                    m_Managers;
    NodeTable* const                m_pNodeTable;               //  From application wrapper (e.g., emexec)
                                                                //      This is persisted here so that DeviceManager can
                                                                //      build itself based on the content of this table,
                                                                //      during the first boot.  It is not currently used
                                                                //      at any point thereafter -- however, the wrapper
                                                                //      must not delete any of the Node objects which
                                                                //      are referenced in this object, and should not
                                                                //      alter the object itself (just good practice).
    bool                            m_OperatorBoot;             //  if false it is an automatic boot
    PanelInterface* const           m_pPanelInterface;          //  Whoever is controlling us
    ExecRunInfo*                    m_pRunInfo;                 //  pointer to EXEC-8 RunInfo object
    RUNINFOS                        m_RunInfoTable;             //  map of all RunInfo objects
    Status                          m_Status;

    //  private methods
    void                            abortRun( RunInfo* const pRunInfo );
    bool                            assignDIAG$ToRun( Activity* const       pActivity,
                                                      UserRunInfo* const    pRunInfo );
    bool                            assignFileToRun( Activity* const    pActivity,
                                                     UserRunInfo* const pRunInfo,
                                                     const std::string& assignImage,
                                                     const std::string& fileName );
    bool                            assignTPF$ToRun( Activity* const    pActivity,
                                                     UserRunInfo* const pRunInfo );
    bool                            generateUniqueRunId( const std::string&     originalRunId,
                                                            std::string* const  pUniqueRunId ) const;
    void                            terminateExecActivities() const;

    //  private static methods
    static std::string              getTimeStamp();

    //  Worker interface
	bool							workerStart();              // do not invoke on this object
	bool							workerStop( const bool );   // do not invoke on this object
	void							worker();

public:
    //  Note that the node table should not change while this object exists...
    //  Specifically, the contained Node objects must not be deleted until this Exec object is deleted.
    Exec( PanelInterface* const pPanelInterface,
          Configuration* const  pConfiguration,
          NodeTable* const      pNodeTable );
    ~Exec();

    //  public methods
    void                            abortRunDemand( RunInfo* const  pRunInfo,
                                                    const bool      cCommand,
                                                    const bool      rCommand,
                                                    const bool      tCommand );
    void                            abortRunFacilities( RunInfo* const pRunInfo );
    bool                            bootExec( const bool operatorBoot );
    void                            cleanupExec();
    void                            closeUserRun( Activity* const       pActivity,
                                                  UserRunInfo* const    pRunInfo );
    DemandRunInfo*                  createDemandRun( Activity* const                        pActivity,
                                                     const SecurityManager::UserProfile&    userProfile,
                                                     const COUNT                            rsiSessionNumber,
                                                     const std::string&                     rsiSessionName,
                                                     const std::string&                     runId,
                                                     const std::string&                     accountId,
                                                     const std::string&                     userId,
                                                     const std::string&                     projectId,
                                                     const UINT32                           options,
                                                     const char                             schedulingPriority,
                                                     const char                             processorDispatchingPriority );
    bool                            createKeyin( const Word36       routing,
                                                 const std::string& input );
    void                            displaySystemErrorMsg( const StopCode   stopCode,
                                                            const COUNT     session ) const;
    void                            displayTime( const EXECTIME     execTime,
                                                 const bool         longFormat,
                                                 const Word36&      routing );
    std::string                     dump( const UINT32      dumpBits );
    COUNT64                         getExecTime() const;
    void                            getExecTimeTDate( TDate* const pTDate ) const;
    void                            getJumpKeys( std::vector<bool>* const pJumpKeys );
    void                            getRunCounters( COUNT* const    pBatchRunCount,
                                                    COUNT* const    pDemandRunCount,
                                                    COUNT* const    pBacklogRunCount ) const;
    void                            getRunids( LSTRING* const pContainer ) const;
    RunInfo*                        getRunInfo( const std::string&  runId,
                                                const bool          attach ) const;
    void                            getStatusCounters( COUNT* const pRunsWaitingOnStartTime,
                                                       COUNT* const pUnopenedDeadlineRuns,
                                                       COUNT* const pOperationsHold,
                                                       COUNT* const pSOptionHold,
                                                       COUNT* const pActiveRuns,
                                                       COUNT* const pInitialLoadRuns ) const;
    void                            pollUserRunInfoObjects( Activity* const pActivity );
    void                            postNodeChangeEvent( const Node* const pNode );
    void                            printTailSheet( ControlModeRunInfo* const pRunInfo ) const;
    void                            registerAndStartKeyin( KeyinActivity* const pActivity );
    void                            setStatus( const Status status );
    bool                            stopExec( const StopCode stopCode );
    void                            terminateDemandRun( DemandRunInfo* const pRunInfo );

    // public inlines
    inline bool                     areBatchRunsHeld() const                { return m_HoldBatchRuns; }
    inline bool                     areDemandRunsHeld() const               { return m_HoldDemandRuns; }
    inline bool                     areDemandTerminalsHeld() const          { return m_HoldDemandTerminals; }
    inline const Configuration&     getConfiguration() const                { return *m_pConfiguration; }
    inline COUNT                    getCurrentSession() const               { return m_CurrentSession; }
    inline COUNT64                  getExecTimeMicroseconds() const
    {
        return SystemTime::getMicrosecondsSinceEpoch() + m_ExecTimeOffsetMicros;
    }
    inline bool                     getJumpKey( const PanelInterface::JUMPKEY jumpKey ) const
    {
        return m_pPanelInterface->getJumpKey( jumpKey );
    }
    inline StopCode                 getLastStopCode() const                 { return m_LastStopCode; }
    inline ExecManager*             getManager( const ManagerId id ) const  { return m_Managers[id]; }
    inline const NodeTable&         getNodeTable() const                    { return *m_pNodeTable; }
    inline ExecRunInfo*             getRunInfo() const                      { return m_pRunInfo; }
    inline Status                   getStatus() const                       { return m_Status; }
    inline Task*                    getTask() const                         { return m_pRunInfo->    getTask(); }
    inline bool                     isActive() const                        { return Worker::isWorkerActive(); }
    inline bool                     isInitialBoot() const                   { return m_InitialBoot; }
    inline bool                     isOperatorBoot() const                  { return m_OperatorBoot; }
    inline bool                     isRunning() const
    {
        return (m_Status != ST_NOT_BOOTED) && (m_Status != ST_STOPPED);
    }
    inline bool                     registerConsole( ConsoleInterface* const    pConsoleInterface,
                                                     const bool                 mainConsole )
    {
        return dynamic_cast<ConsoleManager*>(m_Managers[MID_CONSOLE_MANAGER])->
                registerConsole( pConsoleInterface, mainConsole );
    }
    inline void                     setHoldBatchRuns( const bool hold )
    {
        m_HoldBatchRuns = hold;
    }
    inline void                     setHoldDemandRuns( const bool hold )
    {
        m_HoldDemandRuns = hold;
    }
    inline void                     setHoldDemandTerminals( const bool hold )
    {
        m_HoldDemandTerminals = hold;
    }
    inline void setJumpKey( const PanelInterface::JUMPKEY   jumpKey,
                            const bool                      value ) const
    {
        m_pPanelInterface->setJumpKey( jumpKey, value );
    }
    inline bool                     unregisterConsole( ConsoleInterface* const pConsoleInterface )
    {
        return dynamic_cast<ConsoleManager*>(m_Managers[MID_CONSOLE_MANAGER])->unregisterConsole( pConsoleInterface );
    }

    // public static methods
    static const char*              getStatusString( const Status status );
    static const char*              getStopCodeString( const StopCode stopCode );
    static bool                     isValidAccountId( const std::string& );
    static bool                     isValidFilename( const std::string& );
    static bool                     isValidProjectId( const std::string& );
    static bool                     isValidQualifier( const std::string& );
    static bool                     isValidRunId( const std::string& );
    static bool                     isValidRWKey( const std::string& );
    static bool                     isValidUserId( const std::string& );
};



#endif

