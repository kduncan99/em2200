//  RunInfo class declaration
//  Copyright (c) 2015 by Kurt Duncan
//
//  Whoever needs to use this, should attach() it (which basically just locks it),
//  and then detach() it when done (which unlocks it).



#ifndef     EXECLIB_RUN_INFO_H
#define     EXECLIB_RUN_INFO_H



#include    "FacilityItem.h"
#include    "FileSpecification.h"
#include    "RunConditionWord.h"
#include    "SecurityContext.h"
#include    "Task.h"



//  annoying forward-references
class   Exec;

/*  TODO
THE FOLLOWING APPLIES TO TASK/ACTIVITIES.  WE WILL TRY TO APPLY IT TO CONTROL MODE WHERE POSSIBLE...

The operator X and E keyins cause actions similar to the ABORT$ (3.3) and EABT$
(3.4) RequestCount. Programs or activities can not register for contingencies generated
by these keyins. Common data banks can register for abnormal termination
contingencies (type 017) and TRMRG$ to capture an E keyin, but not an X keyin.
-----
When an activity error terminates, it does not mean immediate termination of all
activities of a multiactivity program, although a program is usually unable to proceed
further.
An activity error termination produces a diagnostic message in the run's print file. The
message identifies the error, where the error occurred, and activity identification of
either a name or a number. In batch mode, a complete control register dump is
provided. In demand mode, the control register dump is optional.
When an activity error terminates, bits are set in the run condition word (see 8.1). For
demand programs, runstream processing continues as usual. For batch programs, once
an activity error terminates, continued runstream processing depends on whether the
last activity of the program error terminated or not.
* If the last activity of a batch program error terminates, further runstream processing
is limited to a postmortem dump (PMD), provided the appropriate postmortem dump
processor call statement is the next control statement or the appropriate options
were specified on the @RUN statement. See the PMD Processor Programming
Reference.
* When an activity other than the last activity error terminates, the error bit (bit 10) is
still set in the condition word. However, another activity of the same program that
later terminates normally through an ER EXIT$ causes the bits to be cleared and bit
11 of the condition word to be set to indicate that some activity in the past has
terminated in error. In this case, batch runstream processing may continue beyond
the point at which the error has occurred.
For this reason, an ER ERR$ in a multiactivity program does not guarantee that
runstream processing will be terminated at the point when an error occurs. To
guarantee runstream termination, use an ER EABT$ (when a PMD is desired) or ER
ABORT$ (when no PMD is desired).

Table B-1. Error Types and Codes
Contingency     Error   Error   Auxilary        Description                     Mnemonic
    Type        Type    Code    Information
    010         02      0       Four ASCII      Remote BREAK key @@XC[,data]    RBK
                                chars or binary
                                zero if no data
    010         02      01      Four ASCII      Remote BREAK key @@XC[,data]    RBK
                                chars or binary
                                zero if no data
    012         01      Table                   I/O call error                  IO
                        B-4,
                        B-5
    012         02      Table                   Symbiont call error             SYMB
                        B-6
    012         03      0                       Program executed ER ERR$        ERR$


Table B-13. Contingency Type 017, Error type 0, Error Code 0
Auxiliary Information - Description
1 Maximum time estimate exceeded.
2 QUOTA limit exceeded.
3 Not used.
4 All activities are in a wait state that can only be cleared by a user activity.
5 E-keyin or @@X T from a demand terminal.
6 Program Control Table (PCT) overflow.
7 Real time program PCT overflow.
010 ER-EABT$ or ER-ABORT$.
011 Not used.
012 A control table other than the PCT has overflowed.
013 Lack of mass storage.
014 Security violation.
015 Attempt to assign a file using incorrect keys.
016 Checkpoint error.
017 Tape labelling detected an error.
020 X-keyin.
*/



class   RunInfo : public Lockable
{
public:
    enum    State
    {
        STATE_IN_BACKLOG,   //  There is a READ$ file in input queue
                            //      Transition to ST_ACTIVE or ST_FIN
                            //  TODO many versions of IN_BACKLOG needed?
        STATE_ACTIVE,       //  We are scheduling (or a candidate for scheduling)
                            //      Transition to ST_FIN or ST_RSI_FIN at end of job
        STATE_FIN,          //  @FIN processed (or some other means of ending the run)
                            //      Transition to ST_SMOQUE or ST_RSI_TERM
        STATE_RSI_TERM,     //  Waiting for RSIManager to let go of us
                            //      Transition to ST_RSI_DETACHED
        STATE_RSI_DETACHED, //  RSIManager let go of us, ready to move on
                            //      Transition to ST_SMOQUE
        STATE_SMOQUE,       //  File(s) (may) exist in SMOQUE for this run
                            //      Get deleted, eventually
    };

    enum    Status
    {
        STATUS_NO_ERROR,    //  No error or abort as of yet
        STATUS_ERROR,       //  error status
        STATUS_FAC_ERROR,   //  facilities error
        STATUS_ABORT,       //  abort status
    };

    class  Counters
    {
    public:
        COUNT64             m_DiskRequestCount;
        COUNT64             m_DiskWordCount;
        COUNT               m_ImagesRead;
        COUNT               m_PagesPrinted;
        COUNT64             m_ResourceWaitMilliseconds;
        COUNT64             m_TapeRequestCount;
        COUNT64             m_TapeWordCount;

        Counters()
            :m_DiskRequestCount( 0 ),
            m_DiskWordCount( 0 ),
            m_ImagesRead( 0 ),
            m_PagesPrinted( 0 ),
            m_ResourceWaitMilliseconds( 0 ),
            m_TapeRequestCount( 0 ),
            m_TapeWordCount( 0 )
        {}
    };

    class   NameItem
    {
        //  Ties an internal name (formatted as a filename, sans qual, keys, cycle, etc) to either
        //  another internal name, or to a facility item (or to nothing).
    public:
        FacilityItem::IDENTIFIER        m_FacilityItemIdentifier;   //  identifies the associated FacItem, if one exists
        const FileSpecification         m_FileSpecification;        //  represents external name given on @USE
        const bool                      m_IOptionSpecified;         //  an I options was given on the @USE which created this
        const SuperString               m_Name;                     //  internal name of interest

        NameItem( const SuperString&                    name,
                  const FileSpecification               fileSpecification,
                  const bool                            iOptionSpecified )
                  :m_FacilityItemIdentifier( FacilityItem::INVALID_IDENTIFIER ),
                  m_FileSpecification( fileSpecification ),
                  m_IOptionSpecified( iOptionSpecified ),
                  m_Name( name )
        {}
    };

    typedef     std::map<SuperString, NameItem*>    NAMEITEMS;
    typedef     NAMEITEMS::iterator                 ITNAMEITEMS;
    typedef     NAMEITEMS::const_iterator           CITNAMEITEMS;


protected:
    //  mostly-static stuff, which is set just once for the run
    const std::string           m_AccountId;
    const std::string           m_ActualRunId;
    Exec* const                 m_pExec;
    const UINT32                m_Options;                      //  Options on @RUN image
    const std::string           m_OriginalRunId;
    const char                  m_ProcessorDispatchingPriority; //  Maintained but not used
    const std::string           m_ProjectId;
    const char                  m_SchedulingPriority;
    SecurityContext*            m_pSecurityContext;             //  Created by whoever creates us, owned by us
    const EXECTIME              m_StartTime;
    const std::string           m_UserId;

    //  dynamic stuff which (possibly) changes over the course of the run
    COUNT                       m_AttachedCount;
    std::string                 m_DefaultQualifier;             //  Initialized to project ID, changed by @QUAL
    FACITEMS                    m_FacItems;                     //  Container of facility items, including assigned file info
    std::string                 m_ImpliedQualifier;             //  Initialized to project ID, changed by @QUAL
    NAMEITEMS                   m_NameItems;                    //  populated by @USE
    RunConditionWord            m_RunConditionWord;
    Task*                       m_pTask;                        //  pointer to Task object
    State                       m_State;
    Status                      m_Status;

    //  counters
    Counters                    m_Counters;

    //  protected constructor
    RunInfo( Exec* const            pExec,
             const State            initialState,
             const std::string&     originalRunId,
             const std::string&     actualRunId,
             const std::string&     accountId,
             const std::string&     projectId,
             const std::string&     userId,
             const UINT32           options,
             const char             schedulingPriority,
             const char             processorDispatchingPriority );

public:
    virtual ~RunInfo();

    //  thread-protection
    inline void attach()
    {
        lock();
        ++m_AttachedCount;
    }

    inline void detach()
    {
        assert(m_AttachedCount > 0);
        --m_AttachedCount;
        unlock();
    }

    //  functional
    bool                        appendTaskActivity( Activity* const pActivity );
    virtual void                cleanActivities();
    void                        establishNameItem( const SuperString&   internalName,
                                                   NameItem* const      pNameItem );
    FacilityItem*               getFacilityItem( const FacilityItem::IDENTIFIER ) const;
    NameItem*                   getNameItem( const SuperString& internalName ) const;
    virtual bool                hasLiveActivity() const;
    virtual void                killActivities() const;

    //  getters
    inline const std::string&       getActualRunId() const              { return m_ActualRunId; }
    inline const std::string&       getAccountId() const                { return m_AccountId; }
    inline const Counters&          getCounters() const                 { return m_Counters; }
    inline const std::string&       getDefaultQualifier() const         { return m_DefaultQualifier; }
    inline const FACITEMS&          getFacilityItems() const            { return m_FacItems; }
    inline const std::string&       getImpliedQualifier() const         { return m_ImpliedQualifier; }
    inline const NAMEITEMS&         getNameItems() const                { return m_NameItems; }
    inline const std::string&       getOriginalRunId() const            { return m_OriginalRunId; }
    inline const std::string&       getProjectId() const                { return m_ProjectId; }
    inline RunConditionWord&        getRunConditionWord()               { return m_RunConditionWord; }
    inline SecurityContext*         getSecurityContext() const          { return m_pSecurityContext; }
    inline const EXECTIME           getStartTime() const                { return m_StartTime; }
    inline State                    getState() const                    { return m_State; }
    inline Status                   getStatus() const                   { return m_Status; }
    inline Task*                    getTask() const                     { return m_pTask; }
    inline const std::string&       getUserId() const                   { return m_UserId; }

    //  setters, etc
    void                        discardNameItem( const SuperString internalName )
    {
        ITNAMEITEMS itni = m_NameItems.find( internalName );
        if ( itni != m_NameItems.end() )
            m_NameItems.erase( itni );
    }

    void                        discardAllNameItems()
    {
        m_NameItems.clear();
    }

    void                        insertFacilityItem( FacilityItem* const pFacItem );

    inline void                 incrementDiskRequestCount( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_DiskRequestCount += increase;
    }

    inline void                 incrementDiskWordCount( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_DiskWordCount += increase;
    }

    inline void                 incrementImagesRead( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_ImagesRead += increase;
    }

    inline void                 incrementPagesPrinted( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_PagesPrinted += increase;
    }

    inline void                 incrementResourceWaitMilliseconds( const COUNT64 increase )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_ResourceWaitMilliseconds += increase;
    }

    inline void                 incrementTapeRequestCount( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_TapeRequestCount += increase;
    }

    inline void                 incrementTapeWordCount( const COUNT increase = 1 )
    {
        assert( m_AttachedCount > 0 );
        m_Counters.m_TapeWordCount += increase;
    }

    inline bool                 isAttached() const          { return (m_AttachedCount > 0); }

    inline void                 removeFacilityItem( const FacilityItem::IDENTIFIER identifier )
    {
        m_FacItems.erase(identifier);
    }

    inline void                 setDefaultQualifier( const std::string& qualifier )
    {
        assert( m_AttachedCount > 0 );
        m_DefaultQualifier = qualifier;
    }

    inline void                 setImpliedQualifier( const std::string& qualifier )
    {
        assert( m_AttachedCount > 0 );
        m_ImpliedQualifier = qualifier;
    }

    inline void                 setRunConditionWord( const UINT64& value )
    {
        assert( m_AttachedCount > 0 );
        m_RunConditionWord.setW( value );
    }

    inline void                 setState( const State state )
    {
        assert( m_AttachedCount > 0 );
        m_State = state;
    }

    inline void                 setStatus( const Status status )
    {
        assert( m_AttachedCount > 0 );
        m_Status = status;
    }

    inline void setSecurityContext( SecurityContext* const pSecurityContext )
    {
        delete m_pSecurityContext;
        m_pSecurityContext = pSecurityContext;
    }

    //  virtuals
    virtual void                dump( std::ostream&        stream,
                                      const std::string&  prefix,
                                      const DUMPBITS      dumpBits ) = 0;
    virtual bool inControlMode() const                                                      { return false; }
    virtual bool isBatch() const                                                            { return false; }
    virtual bool isDemand() const                                                           { return false; }
    virtual bool isExec() const                                                             { return false; }
    virtual bool isTIP() const                                                              { return false; }
    virtual bool isPrivileged() const = 0;

    //  statics
    static std::string          getStateString( const State state );
    static std::string          getStatusString( const Status status );
};



#endif
