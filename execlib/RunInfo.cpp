//  RunInfo implementation



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors

RunInfo::RunInfo
(
Exec* const             pExec,
const State             initialState,
const std::string&      originalRunId,
const std::string&      actualRunId,
const std::string&      accountId,
const std::string&      projectId,
const std::string&      userId,
const UINT32            options,
const char              schedulingPriority,
const char              processorDispatchingPriority
)
:m_AccountId( accountId ),
m_ActualRunId( actualRunId ),
m_pExec( pExec ),
m_Options( options ),
m_OriginalRunId( originalRunId ),
m_ProcessorDispatchingPriority( processorDispatchingPriority ),
m_ProjectId( projectId ),
m_SchedulingPriority( schedulingPriority ),
m_pSecurityContext( 0 ),
m_StartTime( pExec->getExecTime() ),
m_UserId( userId ),
m_State( initialState )
{
    m_AttachedCount = 0;

    //  Set default and implied qualifiers to project ID
    m_DefaultQualifier = projectId;
    m_ImpliedQualifier = projectId;

    //  Other set up stuff
    m_Status = STATUS_NO_ERROR;
    m_pTask = 0;
}


RunInfo::~RunInfo()
{
    if ( m_pTask )
        delete m_pTask;
    delete m_pSecurityContext;
}



//  public methods

//  appendTaskActivity()
//
//  Adds an Activity entry to the current Task object
//
//  Returns:
//      true generally, false if there is no current task
bool
RunInfo::appendTaskActivity
(
    Activity* const     pActivity
)
{
    bool result = false;
    lock();

    if ( m_pTask )
    {
        m_pTask->appendActivity( pActivity );
        result = true;
    }

    unlock();
    return result;
}


//  cleanActivites()
//
//  Called occasionally by the Exec, so we can get rid of zombies.
void
RunInfo::cleanActivities()
{
    lock();
    if ( m_pTask )
        m_pTask->cleanActivities();
    unlock();
}


//  dump()
//
//  For debugging
void
RunInfo::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    lock();

    SystemTime* pStartTime = SystemTime::createFromMicroseconds( m_StartTime );

    //  mostly-static stuff
    stream << prefix << "  Actual RunId:           " << m_ActualRunId << std::endl;
    stream << prefix << "  Original RunId:         " << m_OriginalRunId << std::endl;
    stream << prefix << "  Account Id:             " << m_AccountId << std::endl;
    stream << prefix << "  Project Id:             " << m_ProjectId << std::endl;
    stream << prefix << "  User Id:                " << m_UserId << std::endl;
    stream << prefix << "  @RUN options:           " << execGetOptionsString( m_Options ) << std::endl;
    stream << prefix << "  Scheduling Priority:    " << m_SchedulingPriority << std::endl;
    stream << prefix << "  ProcDispatchPriority:   " << m_ProcessorDispatchingPriority << std::endl;
    stream << prefix << "  AttachedCount:          " << m_AttachedCount << std::endl;
    stream << prefix << "  Start Time:             "
        << std::setw( 4 ) << std::setfill( '0' ) << pStartTime->getYear() << "-"
        << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getMonth() << "-"
        << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getDay()
        << " at "
        << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getHour() << ":"
        << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getMinute() << ":"
        << std::setw( 2 ) << std::setfill( '0' ) << pStartTime->getSecond() << "."
        << std::setw( 4 ) << std::setfill( '0' ) << pStartTime->getMillisecond()
        <<  std::endl;

    //  dynamic stuff
    stream << prefix << "  State:                  " << getStateString( m_State ) << std::endl;
    stream << prefix << "  Status:                 " << getStatusString( m_Status ) << std::endl;
    stream << prefix << "  Default Qualifier:      " << m_DefaultQualifier << std::endl;
    stream << prefix << "  Implied Qualifier:      " << m_ImpliedQualifier << std::endl;
    stream << prefix << "  Run Condition Word:     " << m_RunConditionWord.getW() << std::endl;

    //  Dump name items
    stream << prefix << "  Name Items:" << std::endl;
    for ( CITNAMEITEMS itni = m_NameItems.begin(); itni != m_NameItems.end(); ++itni )
    {
        const NameItem* pNameItem = itni->second;
        std::string intName = pNameItem->m_Name;
        intName.resize( 12, ' ' );

        std::stringstream ioptStrm;
        if ( pNameItem->m_IOptionSpecified )
            ioptStrm << " (IOpt) ";

        std::stringstream linkStrm;
        if ( pNameItem->m_FacilityItemIdentifier != FacilityItem::INVALID_IDENTIFIER )
            linkStrm << " (FacItem id=0" << std::oct << pNameItem->m_FacilityItemIdentifier << ") ";
        else
            linkStrm << " (Orphan) ";

        stream << prefix << "    " << intName << ioptStrm.str() << linkStrm.str()
            << "spec=" << pNameItem->m_FileSpecification << std::endl;
    }

    //  Dump Facility items
    stream << prefix << "  Facility Items:" << std::endl;
    for ( CITFACITEMS itfi = m_FacItems.begin(); itfi != m_FacItems.end(); ++itfi )
        itfi->second->dump( stream, prefix + "    ", itfi->first );

    //  Dump current Task (if any)
    if ( m_pTask )
        m_pTask->dump( stream, prefix + "  ", dumpBits );

    //  Dump counters
    stream << prefix << "  Counters:" << std::endl;
    stream << prefix << "    Disk Transfers:       " << m_Counters.m_DiskRequestCount << std::endl;
    stream << prefix << "    Disk Words:           " << m_Counters.m_DiskWordCount << std::endl;
    stream << prefix << "    Images Read:          " << m_Counters.m_ImagesRead << std::endl;
    stream << prefix << "    Pages Printed:        " << m_Counters.m_PagesPrinted << std::endl;
    stream << prefix << "    Resource Wait Time:   " << m_Counters.m_ResourceWaitMilliseconds << "msec" << std::endl;
    stream << prefix << "    Tape Transfers:       " << m_Counters.m_TapeRequestCount << std::endl;
    stream << prefix << "    Tape Words:           " << m_Counters.m_TapeWordCount << std::endl;

    delete pStartTime;
    pStartTime = 0;

    unlock();
}


//  establishNameItem()
//
//  Establishes a new NameItem entry, discarding any matching previous entry.
//  We take responsibility for the NameItem, which must have been dynamically allocated by the caller.
void
RunInfo::establishNameItem
(
    const SuperString&   internalName,
    NameItem* const      pNameItem
)
{
    assert( m_AttachedCount > 0 );
    SuperString key = internalName;
    key.foldToUpperCase();

    ITNAMEITEMS itni = m_NameItems.find( key );
    if ( itni != m_NameItems.end() )
    {
        delete itni->second;
        itni->second = pNameItem;
    }
    else
        m_NameItems[key] = pNameItem;
}


//  getFacilityItem()
//
//  Finds a facility item for the caller
FacilityItem*
RunInfo::getFacilityItem
(
    const FacilityItem::IDENTIFIER  identifier
) const
{
    assert( m_AttachedCount > 0 );
    CITFACITEMS itfi = m_FacItems.find( identifier );
    return ( itfi == m_FacItems.end() ? 0 : itfi->second );
}


//  getNameItem()
//
//  Finds a name item for the caller
RunInfo::NameItem*
RunInfo::getNameItem
(
    const SuperString&          internalName
) const
{
    assert( m_AttachedCount > 0 );
    for ( CITNAMEITEMS itni = m_NameItems.begin(); itni != m_NameItems.end(); ++itni )
    {
        if ( internalName.compare( itni->second->m_Name ) == 0 )
            return itni->second;
    }

    return 0;
}


//  hasLiveActivity()
//
//  Indicates whether this RunInfo has an activity that has not yet terminated
bool
RunInfo::hasLiveActivity() const
{
    bool result = false;
    lock();
    if ( m_pTask )
        result = m_pTask->hasLiveActivity();
    unlock();
    return result;
}


//  insertFacilityItem()
void
RunInfo::insertFacilityItem
(
    FacilityItem* const     pFacItem
)
{
    assert( m_AttachedCount > 0 );

    //  Need to generate a unique identifier.
    //  Algorithm is such:
    //  If container is empty, use 01.
    //  Else, use (largest value + 1).
    FacilityItem::IDENTIFIER newID = 1;
    if ( !m_FacItems.empty() )
    {
        CITFACITEMS itfi = m_FacItems.end();
        --itfi;
        newID = itfi->first + 1;
    }

    pFacItem->setIdentifier( newID );
    m_FacItems[newID] = pFacItem;
}


//  killActivities()
//
//  Called by Exec during shut down.  Kill any still-active activities, and wait for them to die.
void
RunInfo::killActivities() const
{
    lock();
    if ( m_pTask )
        m_pTask->killActivities();
    unlock();
}



//  statics

//  getStateString()
std::string
RunInfo::getStateString
(
    const State         state
)
{
    switch ( state )
    {
    case STATE_IN_BACKLOG:      return "IN BACKLOG";
    case STATE_ACTIVE:          return "ACTIVE";
    case STATE_RSI_TERM:        return "RSI_TERM";
    case STATE_RSI_DETACHED:    return "RSI_DETACHED";
    case STATE_FIN:             return "FIN";
    case STATE_SMOQUE:          return "IN_QUEUE";
    }

    return "???";
}


//  getStatusString()
std::string
RunInfo::getStatusString
(
    const Status        status
)
{
    switch ( status )
    {
    case STATUS_NO_ERROR:   return "NO ERROR";
    case STATUS_ERROR:      return "ERROR";
    case STATUS_FAC_ERROR:  return "FAC ERROR";
    case STATUS_ABORT:      return "ABORT";
    }

    return "???";
}

