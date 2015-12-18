//  IoManager.h header file



#ifndef     EMEXEC_IO_MANAGER_H
#define     EMEXEC_IO_MANAGER_H



#include    "Activity.h"
#include    "ExecManager.h"
#include    "DiskFacilityItem.h"
#include    "FacilityItem.h"
#include    "TapeFacilityItem.h"



//TODO need to handle region locks on disk files...
//TODO:TAPE this is where we handle tape labeling.  So, DO it.



class   IoManager : public ExecManager
{
public:
    class   IoPacket
    {
    public:
        Activity*               m_pActivity;                //  activity to be notified when this IO is complete
        Activity*               m_pOwnerActivity;           //  activity identifying the original requestor for the IO
        FacilityItem*           m_pFacItem;
        ExecIoFunction          m_Function;
        ExecIoStatus            m_Status;
        COUNT                   m_AbnormalFrameCount;
        COUNT                   m_FinalWordCount;
        UINT64                  m_Address;                  //  word address for word-addressable mass storage file
                                                            //  sector address for sector-addressable mass storage file
        IoAccessControlList     m_AccessControlList;

        IoPacket()
            :m_pActivity( 0 ),
            m_pOwnerActivity( 0 ),
            m_pFacItem( 0 ),
            m_AbnormalFrameCount( 0 ),
            m_FinalWordCount( 0 ),
            m_Address( 0 )
        {}
    };

private:
    //  If we encountered an IO error on a child IO, then we post a console read-reply message
    //  allowing the operator to decide what should be done.  While the message is outstanding,
    //  this object exists, referenced by the request tracker for the request.
    class   ConsoleMessageInfo
    {
    public:
        ConsoleManager::ReadReplyRequest*   m_pConsolePacket;
        std::string                         m_AcceptedResponses;    //  Accepted responses - e.g., "ABGM"
        std::string                         m_Message;

        ConsoleMessageInfo()
            :m_pConsolePacket( 0 )
        {}

        ~ConsoleMessageInfo()
        {
            delete m_pConsolePacket;
        }
    };

    typedef     std::list<IoPacket*>            PACKETS;
    typedef     PACKETS::iterator               ITPACKETS;
    typedef     PACKETS::const_iterator         CITPACKETS;

    //  This is the packet IoManager uses internally to track a requestor IO,
    //  and any child IO which might be in progress to service the requestor.
    class   RequestTracker
    {
    public:
        enum State
        {
            RTST_CHILD_IO_DONE,
            RTST_CHILD_IO_IN_PROGRESS,
            RTST_CHILD_IO_READY,
            RTST_CHILD_IO_SETUP,
            RTST_COMPLETED,
            RTST_CONSOLE_MESSAGE_PENDING,
            RTST_NEW,
        };

        enum Type
        {
            RTTYPE_MASS_STORAGE,
            RTTYPE_TAPE,
        };

        ChannelModule::ChannelProgram*  m_pChannelProgram;      //  child IO (which may be in-progress)
        const DeviceManager::Path*      m_pChildIoPath;         //  path for child IO (if any)
        ConsoleMessageInfo*             m_pConsoleMessageInfo;  //  outstanding read-reply message info (if any)
        IoPacket*                       m_pIoPacket;            //  pointer to caller's IoPacket (0 if completed)
        ExecIoStatus                    m_PendingStatus;        //  during error handling, this is the status that will be returned
                                                                //      in the IO packet for B or G responses.
        bool                            m_RetryFlag;            //  true if we've failed at least one attempt at a child IO
        State                           m_State;
        const Type                      m_Type;

        RequestTracker( const RequestTracker::Type  type,
                        IoPacket* const             pIoPacket,
                        Activity* const             pIoActivity )
            :m_pChannelProgram( new ChannelModule::ChannelProgram( pIoActivity ) ),
            m_pChildIoPath( 0 ),
            m_pConsoleMessageInfo( 0 ),
            m_pIoPacket( pIoPacket ),
            m_PendingStatus( EXIOSTAT_SUCCESSFUL ),
            m_RetryFlag( false ),
            m_State( RTST_NEW ),
            m_Type( type )
        {}

        virtual ~RequestTracker()
        {
            delete m_pChannelProgram;
        }
    };

    class   MassStorageRequestTracker : public RequestTracker
    {
    public:
        bool                            m_AllocationDone;           //  For writes and acquires, this indicates the acquire is done
        Word36*                         m_pChildBuffer;             //  pointer to temporary buffer for child IO, only if necessary
        bool                            m_ChildBufferNeeded;        //  The next child IO is a partial transfer and needs a temp buffer.
        DeviceManager::DEVICE_ID        m_ChildIoDeviceId;          //  DEVICE_ID for the next child IO
        PREP_FACTOR                     m_ChildIoPrepFactor;        //  prep factor of pack associated with DEVICE_ID for next child IO
        DiskFacilityItem* const         m_pDiskItem;                //  dynamic-casted pointer to fac item
        WORD_ID                         m_NextWordAddress;          //  next file-relative word address to be transferred
        TRACK_ID                        m_PendingDiskTrackId;       //  device-relative track ID for current/next child IO
        LDATINDEX                       m_PendingLDATIndex;         //  LDAT for the current/next child IO
        WORD_COUNT                      m_RemainingWordCount;       //  remaining number of words to be transferred
        WORD_ID                         m_StartingWordAddress;      //  first file-relative word address to be transferred
        WORD_COUNT                      m_TotalWordCount;           //  total number of words to be transferred
        IoAccessControlList::Iterator   m_itUserACW;                //  ACW in user packet containing next word to be transferred

        MassStorageRequestTracker( IoPacket* const      pIoPacket,
                                   Activity* const      pIoActivity )
            :RequestTracker( RequestTracker::RTTYPE_MASS_STORAGE, pIoPacket, pIoActivity ),
            m_pDiskItem( dynamic_cast<DiskFacilityItem*>( pIoPacket->m_pFacItem ) )
        {
            m_AllocationDone = false;
            m_pChildBuffer = 0;
            m_ChildBufferNeeded = false;
            m_ChildIoDeviceId = 0;
            m_ChildIoPrepFactor = 0;
            m_NextWordAddress = 0;
            m_RemainingWordCount = 0;
            m_StartingWordAddress = 0;
            m_TotalWordCount = 0;
            m_itUserACW = pIoPacket->m_AccessControlList.begin();
        }

        ~MassStorageRequestTracker()
        {
            assert( m_pChildBuffer == 0 );  //  potential debugging just for now...
        }
    };

    class   TapeRequestTracker : public RequestTracker
    {
    public:
        TapeFacilityItem* const     m_pTapeItem;

        TapeRequestTracker( IoPacket* const     pIoPacket,
                            Activity* const     pIoActivity )
            :RequestTracker( RequestTracker::RTTYPE_TAPE, pIoPacket, pIoActivity ),
            m_pTapeItem( dynamic_cast<TapeFacilityItem*>( pIoPacket->m_pFacItem ) )
        {}
    };

    typedef     std::list<RequestTracker*>      REQUESTS;
    typedef     REQUESTS::iterator              ITREQUESTS;
    typedef     REQUESTS::const_iterator        CITREQUESTS;

    typedef     std::set<Word36*>               CHILDBUFFERS;
    typedef     CHILDBUFFERS::iterator          ITCHILDBUFFERS;
    typedef     CHILDBUFFERS::const_iterator    CITCHILDBUFFERS;


    //  private data
    CHILDBUFFERS                m_ChildBuffersAvailable;
    CHILDBUFFERS                m_ChildBuffersInUse;
    ConsoleManager* const       m_pConsoleManager;
    DeviceManager* const        m_pDeviceManager;
    Activity*                   m_pIoActivity;
    MFDManager* const           m_pMFDManager;
    REQUESTS                    m_PendingRequests;

    //  private static data
    static const COUNT          m_ConcurrentDiskIos = 16;       //  Some day this might be tunable...

    //  private functions
    //  TODO: Many of these can probably be const...?
    void                        allocateSpace( MassStorageRequestTracker* const pTracker );
    bool                        attachChildBuffer( MassStorageRequestTracker* const pTracker );
    void                        detachChildBuffer( MassStorageRequestTracker* const pTracker );
    bool                        pollChildIoDone( RequestTracker* const pTracker );
    bool                        pollChildIoDoneMassStorage( MassStorageRequestTracker* const pTracker );
    bool                        pollChildIoDoneTape( TapeRequestTracker* const pTracker );
    bool                        pollChildIoInProgress( RequestTracker* const pTracker );
    bool                        pollChildIoReady( RequestTracker* const pTracker );
    bool                        pollChildIoReadyMassStorage( MassStorageRequestTracker* const pTracker );
    bool                        pollChildIoReadyTape( TapeRequestTracker* const pTracker );
    bool                        pollChildIoSetup( RequestTracker* const pTracker );
    bool                        pollChildIoSetupMassStorage( MassStorageRequestTracker* const pTracker );
    bool                        pollChildIoSetupTape( TapeRequestTracker* const pTracker );
    bool                        pollConsoleMessage( RequestTracker* const pTracker );
    bool                        pollNew( RequestTracker* const pTracker );
    bool                        pollNewMassStorage( MassStorageRequestTracker* const pTracker );
    bool                        pollNewTape( TapeRequestTracker* const pTracker );
    void                        postConsoleMessage( RequestTracker* const   pRequestTracker,
                                                    const std::string&      errorMnemonic,
                                                    const std::string&      acceptedResponses ) const;
    void                        postConsoleMessageDetail( RequestTracker* const pTracker ) const;
    void                        repostConsoleMessage( RequestTracker* const pRequestTracker,
                                                      const bool            prependQuery ) const;

    inline void completeTracker( RequestTracker* const pTracker ) const
    {
        pTracker->m_pIoPacket->m_pActivity->signal();
        pTracker->m_pIoPacket = 0;
        pTracker->m_State = RequestTracker::RTST_COMPLETED;
    }

    //  private statics
    static ExecIoStatus         convertMFDResult( const MFDManager::Result& result );
    static const char*          getFunctionMnemonic( const ExecIoFunction function );
    static bool                 isAllocationCandidate( const ExecIoFunction function );
    static bool                 isWriteFunction( const ExecIoFunction function );

public:
    IoManager( Exec* const pExec );
    ~IoManager(){};

    bool                        pollPendingRequests();
    void                        startIo( IoPacket* const pIoPacket );

    inline void setIoActivity( Activity* const pActivity )
    {
        m_pIoActivity = pActivity;
    }

    //  ExecManager interface
    void                        cleanup();
    void                        dump( std::ostream&     stream,
                                      const DUMPBITS    dumpBits );
    void                        shutdown();
    bool                        startup();
    void                        terminate();

    //  statics
    std::string                 getRequestTrackerTypeString( const RequestTracker::Type type );
    std::string                 getRequestTrackerStateString( const RequestTracker::State state );
};



#endif
