//  UserRunInfo implementation



#include    "execlib.h"



//  private, protected methods

//  drainPrintBuffer()
//
//  Drains the current print buffer *IF* it is a file symbiont buffer.
//  If RSI, we do nothing, returning SST_NOT_READY (or SST_IO_ERROR if the session is closed)
//  Should probably overload this in DemandRunInfo
UserRunInfo::SymbiontStatus
    UserRunInfo::drainPrintBuffer()
{
    assert( isAttached() );

    if ( m_SymbiontBufferPrint.size() == 0 )
        return SST_IO_ERROR;

    //TODO:BATCH
    //SymbiontBuffer* psbuff = m_SymbiontBufferPrint.back();
    return SST_NOT_READY;
}


//  loadReadBuffer()
//
//  Reads the next block from the buffer's file *IF* it is a file symbiont buffer.
//  If RSI, we tell RSI we want input, and do nothing otherwise, returning SST_NOT_READY
//  (or SST_IO_ERROR if the session is closed)
UserRunInfo::SymbiontStatus
    UserRunInfo::loadReadBuffer()
{
    assert( isAttached() );

    if ( m_SymbiontBufferRead.size() == 0 )
        return SST_IO_ERROR;

    //TODO:BATCH
    //SymbiontBuffer* psbuff = m_SymbiontBufferRead.back();
    //The following is messy - can we unmessify it?  (possibly overload this in DemandRunInfo?)
    dynamic_cast<DemandRunInfo*>(this)->setInputAllowed( true );
    return SST_NOT_READY;
}



//  constructors, destructors

UserRunInfo::UserRunInfo
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
:RunInfo( pExec,
          initialState,
          originalRunId,
          actualRunId,
          accountId,
          projectId,
          userId,
          options,
          schedulingPriority,
          processorDispatchingPriority )
{}


UserRunInfo::~UserRunInfo()
{
    while ( m_SymbiontBufferPrint.size() > 0 )
    {
        delete m_SymbiontBufferPrint.front();
        m_SymbiontBufferPrint.pop_front();
    }

    while ( m_SymbiontBufferPunch.size() > 0 )
    {
        delete m_SymbiontBufferPunch.front();
        m_SymbiontBufferPunch.pop_front();
    }

    while ( m_SymbiontBufferRead.size() > 0 )
    {
        delete m_SymbiontBufferRead.front();
        m_SymbiontBufferRead.pop_front();
    }
}



//  public methods

//  dump()
//
//  For debugging
void
UserRunInfo::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    lock();

    RunInfo::dump( stream, prefix, dumpBits );

    //  Dump symbiont buffer stacks
    stream << prefix << "  Read Symbiont Stack:" << std::endl;
    for ( CITSYMBIONTBUFFERS itsb = m_SymbiontBufferRead.begin(); itsb != m_SymbiontBufferRead.end(); ++itsb )
    {
        stream << "    BufType=" << SymbiontBuffer::getBufferTypeString( (*itsb)->getBufferType() )
            << " SDFType=" << SymbiontBuffer::getSDFTypeString( (*itsb)->getSDFType() )
            << " CharSet=" << SymbiontBuffer::getCharacterSetString( (*itsb)->getCurrentCharacterSet() )
            << std::endl;
    }

    stream << prefix << "  Print Symbiont Stack:" << std::endl;
    for ( CITSYMBIONTBUFFERS itsb = m_SymbiontBufferPrint.begin(); itsb != m_SymbiontBufferPrint.end(); ++itsb )
    {
        stream << "    BufType=" << SymbiontBuffer::getBufferTypeString( (*itsb)->getBufferType() )
            << " SDFType=" << SymbiontBuffer::getSDFTypeString( (*itsb)->getSDFType() )
            << " CharSet=" << SymbiontBuffer::getCharacterSetString( (*itsb)->getCurrentCharacterSet() )
            << std::endl;
    }

    stream << prefix << "  Punch Symbiont Stack:" << std::endl;
    for ( CITSYMBIONTBUFFERS itsb = m_SymbiontBufferPunch.begin(); itsb != m_SymbiontBufferPunch.end(); ++itsb )
    {
        stream << "    BufType=" << SymbiontBuffer::getBufferTypeString( (*itsb)->getBufferType() )
            << " SDFType=" << SymbiontBuffer::getSDFTypeString( (*itsb)->getSDFType() )
            << " CharSet=" << SymbiontBuffer::getCharacterSetString( (*itsb)->getCurrentCharacterSet() )
            << std::endl;
    }

    //  Dump console log
    stream << prefix << "  Console Log:" << std::endl;
    for ( LCITSTRING its = m_ConsoleLog.begin(); its != m_ConsoleLog.end(); ++its )
        stream << prefix << "    " << *its << std::endl;

    unlock();
}


//  isPrivileged
//
//  Checks to see if this run is privileged.
//  It is privileged if:
//      Run is marked privileged (due to user profile setting)
//      SYS$*DLOC$ is assigned with no inhibits
bool
UserRunInfo::isPrivileged() const
{
    return m_pSecurityContext->isPrivilegedUserId() || m_pSecurityContext->hasSecurityFileAssigned();
}


//  pollFromRead()
//
//  Read from the current input buffer.
//  Filters through the control images, doing proper things.
//  Returns the following:
//      SST_IO_ERROR        if we hit an io error reading the input file, or the RSI session is gone,
//                              or we have run out of input symbiont buffers.
//      SST_END_OF_FILE     if there is no more data to be read from the input file
//      SST_NOT_READY       if no input is available from RSI
//      SST_SUCCESSFUL      if we have a proper SDF data image
//      SST_BAD_FORMAT      if we have a bad SDF control word
//      SST_CONTROL_IMAGE   if we read and processed an SDF control image
UserRunInfo::SymbiontStatus
UserRunInfo::pollFromRead
(
    std::string* const  pText,
    UINT8* const        pCharacterSet
)
{
    assert( isAttached() );

    if ( m_SymbiontBufferRead.size() == 0 )
        return SST_IO_ERROR;
    SymbiontBuffer* psbuff = m_SymbiontBufferRead.back();

    //  If End Of File, stop
    if ( psbuff->isEndOfFile() )
        return SST_END_OF_FILE;

    //  If the buffer is empty, load it.
    if ( psbuff->isExhausted() )
    {
        SymbiontStatus sstat = loadReadBuffer();
        if ( sstat != SST_SUCCESSFUL )
            return sstat;
    }

    //  Get control word from buffer.
    const Word36* pcword = psbuff->readWord();
    if ( pcword == 0 )
    {
        //  should never happen - we just loaded the damn thing.
        SystemLog::write( "UserRunInfo::pollFromRead() could not get control word from buffer" );
        return SST_IO_ERROR;
    }

    //  Is this a control image?  If so, is it valid?
    if ( pcword->isNegative() )
    {
        COUNT type = pcword->getS1();
        COUNT dataWords = pcword->getS2();
        if ( dataWords > psbuff->getRemainingRead() )
            return SST_BAD_FORMAT;

        switch ( type )
        {
        case 040:   //  Bypass control record
        case 043:   //  Fortran V Backspace control record
        case 052:   //  SIR$ Line Change control record
        case 053:   //  CTS/IPF Line Number control record
        case 060:   //  Print control record
        case 061:   //  Special print control record
        case 070:   //  Punch control record
            psbuff->advanceReadIndex( dataWords );
            return SST_CONTROL_IMAGE;

        case 042:   //  character set change control record
            psbuff->setCurrentCharacterSet( static_cast<SymbiontBuffer::CharacterSet>(pcword->getS6()) );
            psbuff->advanceReadIndex( dataWords );
            return SST_CONTROL_IMAGE;

        case 050:   //  Label control record
            psbuff->setSDFType( static_cast<SymbiontBuffer::SDFType>(pcword->getS3()) );
            psbuff->setCurrentCharacterSet( static_cast<SymbiontBuffer::CharacterSet>(pcword->getS6()) );
            psbuff->advanceReadIndex( dataWords );
            return SST_CONTROL_IMAGE;

        case 051:   //  Continuation control record only exists in PRINT$ files
            return SST_BAD_FORMAT;

        case 054:   //  End of Reel record; does not exist in READ$ files
            return SST_BAD_FORMAT;

        case 077:   //  character set change control record
            psbuff->setEndOfFile();
            return SST_CONTROL_IMAGE;

        default:
            return SST_BAD_FORMAT;
        }
    }

    //  This is a data image - deal with it.
    COUNT dataWords = pcword->getT1();
    if ( dataWords > psbuff->getRemainingRead() )
        return SST_BAD_FORMAT;

    Word36* pData = psbuff->getBuffer() + psbuff->getNextRead();
    if ( psbuff->getCurrentCharacterSet() == SymbiontBuffer::CSET_ASCII )
        *pText = miscWord36AsciiToString( pData, dataWords, false );
    else
        *pText = miscWord36FieldataToString( pData, dataWords );
    psbuff->advanceReadIndex( dataWords );

    return SST_SUCCESSFUL;
}


//  post042ToPrint()
//
//  Changes the current character set for the current PRINT$ buffer.
//  If the buffer is full
//      and an RSI buffer, we return SST_NOT_READY.
//      else a file buffer, we write it out to disk and clear it
//  Then we put a 042 character set change control record into the buffer,
//  and change the current character set value for the buffer.
//
//  This work is done regardless of whether the indicated charSet already
//  matches the SymbiontBuffer's current charSet.
UserRunInfo::SymbiontStatus
UserRunInfo::post042ToPrint
(
    const SymbiontBuffer::CharacterSet  charSet
)
{
    assert( isAttached() );

    SymbiontBuffer* psbuff = m_SymbiontBufferPrint.back();
    if ( psbuff->getRemainingWrite() == 0 )
    {
        SymbiontStatus sstat = drainPrintBuffer();
        if ( sstat != SST_SUCCESSFUL )
            return sstat;
    }

    Word36 cword;
    cword.setS1( 042 );
    cword.setS6( charSet );
    psbuff->writeWord( cword );
    psbuff->setCurrentCharacterSet( charSet );
    return SST_SUCCESSFUL;
}


//  post077ToPrint()
//
//  If the buffer is full
//      and an RSI buffer, we return SST_NOT_READY.
//      else a file buffer, we write it out to disk and clear it
//  Then we put a 077 end of file control record into the buffer.
UserRunInfo::SymbiontStatus
UserRunInfo::post077ToPrint()
{
    assert( isAttached() );

    SymbiontBuffer* psbuff = m_SymbiontBufferPrint.back();
    if ( psbuff->getRemainingWrite() == 0 )
    {
        SymbiontStatus sstat = drainPrintBuffer();
        if ( sstat != SST_SUCCESSFUL )
            return sstat;
    }

    Word36 cword;
    cword.setS1( 077 );
    psbuff->writeWord( cword );
    return SST_SUCCESSFUL;
}


//  postInfoToPrint()
//
//  Posts interesting items from the RunInfo object to the RunInfo object's PRINT$
void
UserRunInfo::postInfoToPrint()
{
    assert( isAttached() );

    postToPrint( "  Actual RunId:           " + m_ActualRunId );
    postToPrint( "  Original RunId:         " + m_OriginalRunId );
    postToPrint( "  Account Id:             " + m_AccountId );
    postToPrint( "  Project Id:             " + m_ProjectId );
    postToPrint( "  User Id:                " + m_UserId );
    postToPrint( "  @RUN options:           " + execGetOptionsString( m_Options ) );

    std::stringstream stream;
    stream << "  Scheduling Priority:    " << m_SchedulingPriority;
    postToPrint( stream.str() );

    stream.str( "" );
    stream << "  ProcDispatchPriority:   " << m_ProcessorDispatchingPriority;
    postToPrint( stream.str() );

    SystemTime* pStartTime = SystemTime::createFromMicroseconds( m_pExec->getExecTime() );
    stream.str( "" );
    stream << "  Start Time:             " << pStartTime->getTimeStamp()
            << "." << std::setw(6) << std::setfill( '0' ) << pStartTime->getMicrosecond() <<  std::endl;
    delete pStartTime;
    pStartTime = 0;
    postToPrint( stream.str() );

    //  dynamic stuff
    postToPrint( "  Status:                 " + getStatusString( m_Status ) );
    postToPrint( "  Default Qualifier:      " + m_DefaultQualifier );
    postToPrint( "  Implied Qualifier:      " + m_ImpliedQualifier );
    postToPrint( "  Run Condition Word:     " + m_RunConditionWord.toOctal() );

    postToPrint( "  Name Items:" );
    for ( CITNAMEITEMS itni = m_NameItems.begin(); itni != m_NameItems.end(); ++itni )
    {
        stream.str( "" );
        stream << "    " << itni->first << " -> " << itni->second->m_FileSpecification;
        if ( itni->second->m_FacilityItemIdentifier != FacilityItem::INVALID_IDENTIFIER )
        {
            stream << " [id="
                << std::oct << std::setw( 12 ) << std::setfill( '0' ) << itni->second->m_FacilityItemIdentifier
                << "]";
        }
        if ( itni->second->m_IOptionSpecified )
            stream << " Iopt";
        postToPrint( stream.str() );
    }

    postToPrint( "  Facility Items:" );
    for ( CITFACITEMS itfi = m_FacItems.begin(); itfi != m_FacItems.end(); ++itfi )
    {
        FacilityItem::IDENTIFIER id = itfi->first;
        FacilityItem* pFacItem = itfi->second;
        stream.str( "" );
        stream << "    [" << std::oct << std::setw( 12 ) << std::setfill( '0' ) << id << "] "
            << pFacItem->getQualifier() << "*" << pFacItem->getFileName()
            << "  Opts=" << execGetOptionsString( pFacItem->getAssignOptions() )
            << "  Rel=" << (pFacItem->getReleaseFlag() ? "Y" : "N");
        postToPrint( stream.str() );
    }
}


//  popSymbiontBufferPrint()
//
//  Removes the symbiont buffer pointer from the stack and deletes it.
//  Caller must ensure the buffer is closed out properly before calling here.
bool
UserRunInfo::popSymbiontBufferPrint()
{
    assert( isAttached() );

    if ( m_SymbiontBufferPrint.size() == 0 )
        return false;
    delete m_SymbiontBufferPrint.back();
    m_SymbiontBufferPrint.pop_back();

    return true;
}


//  popSymbiontBufferPunch()
//
//  Removes the symbiont buffer pointer from the stack and deletes it.
//  Caller must ensure the buffer is closed out properly before calling here.
bool
UserRunInfo::popSymbiontBufferPunch()
{
    assert( isAttached() );

    if ( m_SymbiontBufferPunch.size() == 0 )
        return false;
    delete m_SymbiontBufferPunch.back();
    m_SymbiontBufferPunch.pop_back();

    return true;
}


//  popSymbiontBufferRead()
//
//  Removes the symbiont buffer pointer from the stack and deletes it.
//  Caller must ensure the buffer is closed out properly before calling here.
bool
UserRunInfo::popSymbiontBufferRead()
{
    assert( isAttached() );

    if ( m_SymbiontBufferRead.size() == 0 )
        return false;
    delete m_SymbiontBufferRead.back();
    m_SymbiontBufferRead.pop_back();

    return true;
}


//  postToConsoleLog()
//
//  Posts a simple message to the console log
void
UserRunInfo::postToConsoleLog
(
    const std::string&      message
)
{
    assert( isAttached() );

    m_ConsoleLog.push_back( message );
}


//  postToPrint()
//
//  Posts an image to PRINT$
UserRunInfo::SymbiontStatus
UserRunInfo::postToPrint
(
    const SymbiontBuffer::CharacterSet  charSet,
    const std::string&                  text,
    const COUNT                         spacing,
    const bool                          pageEject
)
{
    assert( isAttached() );

    if ( m_SymbiontBufferPrint.size() == 0 )
        return SST_IO_ERROR;

    //  Do we need a character set change record?
    SymbiontBuffer* psbuff = m_SymbiontBufferPrint.back();
    if ( charSet != psbuff->getCurrentCharacterSet() )
    {
        SymbiontStatus sstat = post042ToPrint( charSet );
        if ( sstat != SST_SUCCESSFUL )
            return sstat;
    }

    //  How many data words and image words will this require?
    COUNT divisor = ( charSet == SymbiontBuffer::CSET_ASCII ) ? 4 : 6;
    COUNT dataWords = text.size() / divisor;
    if ( text.size() % divisor != 0 )
        ++dataWords;
    COUNT imageWords = dataWords + 1;
    if ( imageWords > psbuff->getBufferSize() )
    {
        std::stringstream strm;
        strm << "UserRunInfo::postToPrint() imageWords=0" << std::oct << imageWords
            << " which is > bufferSize=0" << std::oct << psbuff->getBufferSize();
        SystemLog::write( strm.str() );
        return SST_BAD_FORMAT;
    }

    //  If we have insufficient space, drain the buffer.
    if ( psbuff->getRemainingWrite() < imageWords )
    {
        SymbiontStatus sstat = drainPrintBuffer();
        if ( sstat != SST_SUCCESSFUL )
            return sstat;
    }

    //  Create control word and write it to the buffer
    Word36 cword;
    switch ( psbuff->getSDFType() )
    {
    case SymbiontBuffer::SDFT_FILE:
        //  BRKPT$ output
        cword.setT1( dataWords );
        cword.setS6( charSet );
        break;

    case SymbiontBuffer::SDFT_PRINT:
        //  Normal PRINT$ output
        cword.setT1( dataWords );
        cword.setT2( pageEject ? 07777 : spacing );
        cword.setS6( charSet );
        break;

    case SymbiontBuffer::SDFT_CARD:
    case SymbiontBuffer::SDFT_FTP:
    case SymbiontBuffer::SDFT_GENERAL:
    case SymbiontBuffer::SDFT_PCIOS:
        //  We should never encounter these types
        return SST_BAD_FORMAT;

    case SymbiontBuffer::SDFT_UNSPECIFIED:
        //  RSI output
        cword.setT1( dataWords );
        break;
    }

    psbuff->writeWord( cword );

    //  Now convert directly into the buffer from the text
    Word36* pData = psbuff->getBuffer() + psbuff->getNextWrite();
    if ( charSet == SymbiontBuffer::CSET_ASCII )
        miscStringToWord36Ascii( text, pData, dataWords );
    else
        miscStringToWord36Fieldata( text, pData, dataWords );
    psbuff->advanceWriteIndex( dataWords );

    return SST_SUCCESSFUL;
}


//  setErrorMode()
//
//  Only for CoarseScheduler's convenience, to handle syntax errors and the like
void
UserRunInfo::setErrorMode()
{
    assert( isAttached() );

    if ( !m_RunConditionWord.anyPreviousTaskInError() )
    {
        std::string consMsg = m_ActualRunId + " ERROR";
        ConsoleManager* pcmgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
        pcmgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );
    }

    if ( m_RunConditionWord.getPreviousTaskError() )
        m_RunConditionWord.setPreviousPreviousTaskError();

    m_RunConditionWord.setPreviousTaskError();
}

