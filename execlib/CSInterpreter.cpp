//  CSInterpreter implementation
//  Copyright (c) 2015 by Kurt Duncan

//  Non control-mode legal use - We do not enforce this - caller must do so
//  Command     CSF Non-TIP     CSF TIP         CSI
//  @ADD            X
//  @ASG            X               X           X
//  @BRKPT          X
//  @CAT            X               X           X
//  @CKPT           X
//  @FREE           X               X           X
//  @LOG            X               X
//  @MODE           X               X           X
//  @QUAL           X               X           X
//  @START          X               X
//  @SYM            X               X
//  @USE            X               X           X



#include    "execlib.h"



//  private methods

//  atEnd()
//
//  Returns true if we have scanned the last character in m_Statement
inline bool
CSInterpreter::atEnd() const
{
    return m_Index >= m_Statement.size();
}


//  atSubfieldTerminator()
//
//  Returs true if the next character to be scanned is a terminator for a subfield.
//  The end of the image is considered a terminator.
inline bool
CSInterpreter::atSubFieldTerminator() const
{
    if ( atEnd() )
        return true;

    switch ( m_Statement[m_Index] )
    {
    case ';':
    case ' ':
    case ',':
    case '/':
        return true;
    }

    return false;
}


//  executeAsg()
//
//  Assigns a file.
inline CSInterpreter::Status
CSInterpreter::executeAsg()
{
    m_FacilitiesResult = m_pFacMgr->asg( m_pActivity,
                                                                m_pSecurityContext,
                                                                m_pRunInfo,
                                                                m_Options,
                                                                m_FileSpec1,
                                                                m_AdditionalFields );
    return CSIST_FACILITIES_RESULT;
}


//  executeCat()
//
//  Catalogs a file.
inline CSInterpreter::Status
CSInterpreter::executeCat()
{
    m_FacilitiesResult = m_pFacMgr->cat( m_pActivity,
                                                                m_pSecurityContext,
                                                                m_pRunInfo,
                                                                m_Options,
                                                                m_FileSpec1,
                                                                m_AdditionalFields );
    return CSIST_FACILITIES_RESULT;
}


//  executeDir()
//
//  Calls into facilities to execute the interpreted @DIR statement
inline CSInterpreter::Status
CSInterpreter::executeDir() const
{
    CSInterpreter::Status stat = CSIST_SUCCESSFUL;

    UINT32 dumpOptions = OPTB_O | OPTB_A | OPTB_F;
    UINT32 listOptions = OPTB_S | OPTB_L | OPTB_M;
    UserRunInfo* puri = dynamic_cast<UserRunInfo*>( m_pActivity->getRunInfo() );

    if ( puri && puri->isPrivileged() )
    {
        if ( m_Options & dumpOptions )
            stat = executeDirDump( puri );
        else if ( m_Options & listOptions )
            stat = executeDirList( puri );
        else
            stat = executeDirCatalog( puri );
    }
    else
        stat = CSIST_NOT_ALLOWED;

    return stat;
}


//  executeDirCatalog()
//
//  Display catalog of all files - use FileSetInfo objects for this (no need for actual MFD data)
CSInterpreter::Status
CSInterpreter::executeDirCatalog
(
    UserRunInfo* const      pUserRunInfo
) const
{
    std::stringstream strm;
    pUserRunInfo->postToPrint( "Cataloged Files:" );
    MFDManager::FileSetInfoList list;
    m_pMfdMgr->getFileSetInfoList( &list );
    for ( MFDManager::FileSetInfoList::const_iterator it = list.begin(); it != list.end(); ++it )
    {
        MFDManager::FileSetInfo* pfsInfo = *it;
        strm.str( "" );
        strm << "  " << pfsInfo->m_Qualifier << "*" << pfsInfo->m_Filename << " [";

        bool commaFlag = false;
        for ( INDEX cx = 0; cx < pfsInfo->m_CycleEntries.size(); ++cx )
        {
            if ( pfsInfo->m_CycleEntries[cx].m_Exists )
            {
                if ( commaFlag )
                    strm << ",";
                else
                    commaFlag = true;
                strm << (pfsInfo->m_CycleEntries[cx].m_ToBeCataloged ? "!" : "")
                    << (pfsInfo->m_CycleEntries[cx].m_ToBeDropped ? "*" : "")
                    << pfsInfo->m_CycleEntries[cx].m_AbsoluteCycle;
            }
        }

        strm << "]";
        pUserRunInfo->postToPrint( strm.str() );
    }

    return CSIST_SUCCESSFUL;
}


//  executeDirDump()
//
//  Display a selected MFD sector to the user
CSInterpreter::Status
CSInterpreter::executeDirDump
(
    UserRunInfo* const      pUserRunInfo
) const
{
    std::stringstream strm;
    strm << "Directory Sector " << std::oct << std::setw( 12 ) << std::setfill( '0' ) << m_Value;
    pUserRunInfo->postToPrint( strm.str() );

    DSADDR dsAddr = static_cast<DSADDR>( m_Value );
    Word36 sector[WORDS_PER_SECTOR];
    MFDManager::Result mfdResult = m_pMfdMgr->getDirectorySector( dsAddr, sector );
    if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
    {
        strm.str( "" );
        strm << "  Sector cannot be retrieved:" << MFDManager::getResultString( mfdResult );
        pUserRunInfo->postToPrint( strm.str() );
        return CSIST_NOT_FOUND;
    }

    for ( INDEX wx = 0; wx < WORDS_PER_SECTOR; wx += 4 )
    {
        strm.str( "" );
        strm << std::oct << std::setw( 3 ) << std::setfill( '0' ) << wx << ":";

        if ( m_Options & OPTB_O )
        {
            for ( INDEX wy = 0; wy < 4; ++wy )
                strm << " " << sector[wx + wy].toOctal();
        }

        if ( m_Options & OPTB_F )
        {
            for ( INDEX wy = 0; wy < 4; ++wy )
                strm << " " << sector[wx + wy].toFieldata();
        }

        if ( m_Options & OPTB_A )
        {
            for ( INDEX wy = 0; wy < 4; ++wy )
                strm << " " << sector[wx + wy].toAscii();
        }

        pUserRunInfo->postToPrint( strm.str() );
    }

    return CSIST_SUCCESSFUL;
}


//  executeDirList()
//
//  List directory sector addresses associated with m_FileSpec1
inline CSInterpreter::Status
CSInterpreter::executeDirList
(
    UserRunInfo* const      pUserRunInfo
) const
{
    std::stringstream strm;
    MFDManager::DSADDRLIST dsAddresses;

    if ( m_Options & OPTB_S )
    {
        //  Generate header and list for search items
        strm << "Search item addresses for " << m_FileSpec1 << ":";
        m_pMfdMgr->getSearchItemAddresses( m_FileSpec1.m_Qualifier, m_FileSpec1.m_FileName, &dsAddresses );
    }
    else if ( m_Options & OPTB_L )
    {
        //  Generate header and list for lead items
        strm << "Lead item addresses for " << m_FileSpec1 << ":";
        //TODO:@DIR
//                MFDManager::Result result = m_pMfdMgr->getSearchItemAddresses( m_FileSpec1.m_Qualifier,
//                                                                                               m_FileSpec1.m_FileName,
//                                                                                               &dsAddresses );
    }
    else if ( m_Options & OPTB_M )
    {
        //  Generate header and list for main items
        strm << "Main item addresses for " << m_FileSpec1 << ":";
        //TODO:@DIR
//                MFDManager::Result result = m_pMfdMgr->getSearchItemAddresses( m_FileSpec1.m_Qualifier,
//                                                                                               m_FileSpec1.m_FileName,
//                                                                                               &dsAddresses );
    }

    //  Print header
    pUserRunInfo->postToPrint( strm.str() );

    //  Print list
    if ( dsAddresses.empty() )
        pUserRunInfo->postToPrint( "  <none>" );
    else
    {
        for ( MFDManager::CITDSADDRLIST itAddr = dsAddresses.begin(); itAddr != dsAddresses.end(); ++itAddr )
        {
            strm.str( "" );
            strm << "  " << std::oct << std::setw( 12 ) << std::setfill( '0' ) << *itAddr;
            pUserRunInfo->postToPrint( strm.str() );
        }
    }

    return CSIST_SUCCESSFUL;
}


//  executeFin()
//
//  Closes out the RunInfo object
inline CSInterpreter::Status
CSInterpreter::executeFin()
{
    if ( m_pRunInfo->isExec() )
        return CSIST_NOT_ALLOWED;
    m_pRunInfo->attach();
    m_pRunInfo->setState( RunInfo::STATE_FIN );
    m_pRunInfo->detach();
    return CSIST_SUCCESSFUL;
}


//  executeFree()
//
//  Calls FacMgr to process the @FREE statement
inline CSInterpreter::Status
CSInterpreter::executeFree()
{
    m_FacilitiesResult = m_pFacMgr->free( m_pActivity,
                                                                 m_pSecurityContext,
                                                                 m_pRunInfo,
                                                                 m_Options,
                                                                 m_FileSpec1 );
    return CSIST_FACILITIES_RESULT;
}


//  executeLog()
//
//  Creates a log entry.
//  Normally, this goes to the Ascii Log; Since we don't have one of those, we just put it in SystemLog.
inline CSInterpreter::Status
CSInterpreter::executeLog()
{
    std::string logStr = m_pRunInfo->getActualRunId();
    logStr += ":";
    logStr += m_Text;
    SystemLog::write( logStr );

    return CSIST_SUCCESSFUL;
}


//  executeMsg()
//
//  Executes the @MSG command.
//  Ignores C, H, I, and S options (which indicate a particular console), since we have only one console.
//  N option basically does nothing.
//  W option waits for operator response.
CSInterpreter::Status
CSInterpreter::executeMsg()
{
    UINT32 allOptions = OPTB_C | OPTB_H | OPTB_I | OPTB_N | OPTB_S | OPTB_W;
    if ( (m_Options | allOptions) != allOptions )
        return CSIST_INVALID_OPTION;

    bool optN = (m_Options & OPTB_N) ? true : false;
    bool optW = (m_Options & OPTB_W) ? true : false;
    if ( optN && optW )
        return CSIST_INVALID_OPTION_COMBINATION;

    if ( !optN )
    {
        std::string prefixedMsg = m_pRunInfo->getActualRunId();
        prefixedMsg += "*MSG:";
        SuperString truncMsg = m_Text.substr( 0, 60 );
        prefixedMsg += truncMsg;
        Word36 routing;
        if ( optW )
        {
            ConsoleManager::ReadReplyRequest request( m_pRunInfo, 0, ConsoleManager::Group::SYSTEM, prefixedMsg, 62 );
            m_pConsMgr->postReadReplyMessage( &request, true );
        }
        else
        {
            m_pConsMgr->postReadOnlyMessage( prefixedMsg, routing, m_pRunInfo );
        }
    }

    return CSIST_SUCCESSFUL;
}


//  executeQual()
//
//  Calls into facilities to execute the interpreted @QUAL statement
inline CSInterpreter::Status
CSInterpreter::executeQual()
{
    m_FacilitiesResult = m_pFacMgr->qual( m_pRunInfo, m_Options, m_Qualifier );
    return CSIST_FACILITIES_RESULT;
}


//  executeRinfo()
//
//  Calls into facilities to execute the interpreted @RINFO statement
inline CSInterpreter::Status
CSInterpreter::executeRinfo()
{
    UserRunInfo* puri = dynamic_cast<UserRunInfo*>( m_pRunInfo );
    if ( puri )
    {
        puri->attach();
        puri->postInfoToPrint();
        puri->detach();
    }
    return CSIST_SUCCESSFUL;
}


//  executeRun()
//
//  Basically, it doesn't execute anything, just complains.
inline CSInterpreter::Status
CSInterpreter::executeRun()
{
    UserRunInfo* puri = dynamic_cast<UserRunInfo*>( m_pRunInfo );
    if ( puri )
    {
        puri->attach();
        puri->postToPrint( "RUN ALREADY ACTIVE" );
        puri->setErrorMode();
        puri->detach();
    }
    return CSIST_SUCCESSFUL;
}


//  executeSetc()
//
//  Updates some portion of the run condition word
CSInterpreter::Status
CSInterpreter::executeSetc()
{
    Word36& rcw = m_pRunInfo->getRunConditionWord();

    //  First, handle A/I/N/P options
    //  A clears bit 5, I sets it
    if ( m_Options & OPTB_I )
        rcw.logicalOr( 010000000000LL );
    else if ( m_Options & OPTB_A )
        rcw.logicalAnd( 0767777777777LL );

    //  N sets bit 3, P clears it
    if ( m_Options & OPTB_N )
        rcw.logicalOr( 040000000000LL );
    else if ( m_Options & OPTB_P )
        rcw.logicalAnd( 0737777777777LL );

    //  Now dink around with the partial-word stuff.
    COUNT value = m_Value & 07777;
    switch ( m_PartialWord )
    {
    case SPAW_T1:
        if ( m_LogicalOp == SLOP_AND )
            value &= rcw.getT1();
        else if ( m_LogicalOp == SLOP_OR )
            value |= rcw.getT1();
        else if ( m_LogicalOp == SLOP_XOR )
            value ^= rcw.getT1();
        //  We are only allowed to update bits 7 and 8
        rcw.setT1( (rcw.getT1() & 07774) | (value & 03) );
        break;

    case SPAW_NONE:     //  defaults to T2
    case SPAW_T2:
        if ( m_LogicalOp == SLOP_AND )
            value &= rcw.getT2();
        else if ( m_LogicalOp == SLOP_OR )
            value |= rcw.getT2();
        else if ( m_LogicalOp == SLOP_XOR )
            value ^= rcw.getT2();
        rcw.setT2( value );
        break;

    case SPAW_S2:
        value &= 077;
        if ( m_LogicalOp == SLOP_AND )
            value &= rcw.getS2();
        else if ( m_LogicalOp == SLOP_OR )
            value |= rcw.getS2();
        else if ( m_LogicalOp == SLOP_XOR )
            value ^= rcw.getS2();
        //  We are only allowed to update bits 7 and 8
        rcw.setS2( (rcw.getS2() & 074) | (value & 03) );
        break;

    case SPAW_S3:
        value &= 077;
        if ( m_LogicalOp == SLOP_AND )
            value &= rcw.getS3();
        else if ( m_LogicalOp == SLOP_OR )
            value |= rcw.getS3();
        else if ( m_LogicalOp == SLOP_XOR )
            value ^= rcw.getS3();
        rcw.setS3( value );
        break;

    case SPAW_S4:
        value &= 077;
        if ( m_LogicalOp == SLOP_AND )
            value &= rcw.getS4();
        else if ( m_LogicalOp == SLOP_OR )
            value |= rcw.getS4();
        else if ( m_LogicalOp == SLOP_XOR )
            value ^= rcw.getS4();
        rcw.setS4( value );
        break;
    }

    return CSIST_SUCCESSFUL;
}


//  executeUse()
//
//  Calls into facilities to execute the interpreted @USE statement
inline CSInterpreter::Status
CSInterpreter::executeUse()
{
    m_FacilitiesResult = m_pFacMgr->use( m_pRunInfo, m_Options, m_FileSpec1, m_FileSpec2 );
    return CSIST_FACILITIES_RESULT;
}


//  initialize()
//
//  Initialize the various data members for a first or subsequent interpretation
inline void
CSInterpreter::initialize()
{
    m_AccountId.clear();
    m_AdditionalFields.clear();
    m_AllowContinuation = false;
    m_AllowLabel = false;
    m_Command = CMD_INIT;
    m_DemandFlag = false;
    m_FacilitiesResult.clear();
    m_FileSpec1.clear();
    m_FileSpec2.clear();
    m_Images.clear();
    m_Index = 0;
    m_Label.clear();
    m_LogicalOp = SLOP_NONE;
    m_Options = 0;
    m_PartialWord = SPAW_NONE;
    m_ProcessorDispatchingPriority = ' ';
    m_ProjectId.clear();
    m_Qualifier.clear();
    m_RunId.clear();
    m_SchedulingPriority = ' ';
    m_Statement.clear();
    m_Status = CSIST_NOT_FOUND;
    m_StopExecutionFlag = false;
    m_Text.clear();
    m_UserId.clear();
    m_Value = 0;
}


//  interpret()
//
//  Common driver code for interpreting a statement - This is where we start...
CSInterpreter::Status
CSInterpreter::interpret()
{
    //  Make sure we have a masterspace
    m_Status = scanMasterSpace();
    if ( m_Status != CSIST_SUCCESSFUL )
        return CSIST_NOT_CONTROL_STATEMENT;

    m_Status = scanWhiteSpace( m_AllowContinuation );
    if ( (m_Status != CSIST_SUCCESSFUL) && (m_Status != CSIST_NOT_FOUND) )
        return m_Status;

    if ( m_AllowLabel )
    {
        //  Look for a potential label.
        m_Status = scanLabel();
        if ( (m_Status != CSIST_SUCCESSFUL) && (m_Status != CSIST_NOT_FOUND) )
            return m_Status;

        m_Status = scanWhiteSpace( m_AllowContinuation );
        if ( (m_Status != CSIST_SUCCESSFUL) && (m_Status != CSIST_NOT_FOUND) )
            return m_Status;
    }

    //  Scan the remainder of the image, as a control statement.
    m_Status = scanRemainder();
    if ( m_Status != CSIST_NOT_FOUND )
        return m_Status;

    //  Scan the remainder of the image, as a processor call.
    //TODO:TASK

    m_Status = CSIST_NOT_FOUND;
    return m_Status;
}


//  scanAbsoluteFileCycle()
//
//  Scans an absolute file cycle from the image's current index.
//  Format is:
//      '('  n [ n [ n ] ] ')'
CSInterpreter::Status
CSInterpreter::scanAbsoluteFileCycle
(
    UINT16* const       pCycle
)
{
    if ( atEnd() || ( m_Statement[m_Index] != '(' ) )
        return CSIST_NOT_FOUND;
    ++m_Index;
    UINT32 cycle = 0;

    COUNT digits = 0;
    bool done = false;
    while ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if ( ch == ')' )
        {
            if ( digits == 0 )
                return CSIST_INVALID_CYCLE;
            done = true;
            ++m_Index;
            break;
        }

        if ( !isdigit( ch ) )
            return CSIST_INVALID_CYCLE;

        cycle = cycle * 10 + ( ch - '0' );
        ++m_Index;
        ++digits;
    }

    if ( !done || ( cycle == 0 ) || ( cycle > 999 ) )
        return CSIST_INVALID_CYCLE;

    *pCycle = static_cast<UINT16>( cycle );
    return CSIST_SUCCESSFUL;
}


//  scanAdditionalFields()
//
//  Call after scanning past leading whitespace
//
//  format:
//      [ ',' [whitespace] field ]*
CSInterpreter::Status
CSInterpreter::scanAdditionalFields()
{
    if ( ( atEnd() ) || ( m_Statement[m_Index] != ',' ) )
        return CSIST_NOT_FOUND;
    ++m_Index;

    std::vector<VSTRING> fields;
    Status stat = scanFields( &fields );
    if ( stat != CSIST_SUCCESSFUL )
        return stat;

    m_AdditionalFields.clear();
    for ( INDEX fx = 0; fx < fields.size(); ++fx )
    {
        m_AdditionalFields.push_back( FacilitiesManager::Field() );
        for ( INDEX sx = 0; sx < fields[fx].size(); ++sx )
            m_AdditionalFields.back().push_back( FacilitiesManager::SubField( fields[fx][sx] ) );
    }

    return CSIST_SUCCESSFUL;
}


//  scanBasicFileSpecification()
//
//  Scans the basic portion of a file spec as follows:
//      fileSpec:
//          [ qualifier ] fileName [ relativeCycle | absoluteCycle ]
CSInterpreter::Status
CSInterpreter::scanBasicFileSpecification
(
    FileSpecification* const    pFileSpec
)
{
    pFileSpec->clear();
    Status stat = scanQualifier( &pFileSpec->m_Qualifier, true );
    if ( stat == CSIST_SUCCESSFUL )
    {
        pFileSpec->m_QualifierSpecified = true;
        pFileSpec->m_Qualifier.foldToUpperCase();
    }
    else if ( stat != CSIST_NOT_FOUND )
        return stat;

    stat = scanFileName( &pFileSpec->m_FileName );
    if ( stat == CSIST_SUCCESSFUL )
        pFileSpec->m_FileName.foldToUpperCase();
    else
        return stat;

    stat = scanRelativeFileCycle( &pFileSpec->m_Cycle );
    if ( stat == CSIST_SUCCESSFUL )
        pFileSpec->m_RelativeCycleSpecified = true;
    else if ( stat == CSIST_NOT_FOUND )
    {
        stat = scanAbsoluteFileCycle( reinterpret_cast<UINT16*>(&pFileSpec->m_Cycle) );
        if ( stat == CSIST_SUCCESSFUL )
            pFileSpec->m_AbsoluteCycleSpecified = true;
    }
    if ( ( stat != CSIST_SUCCESSFUL ) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    return CSIST_SUCCESSFUL;
}


//  scanDirectory()
//
//  Scans a directoryId from the image's current index.
//  We do NOT validate the name of a correctly-formatted directoryId -- normally "STD" or "SHARED" --
//  because one day we might support something other than those two possibilities.
//  Directory Ids consist of 1 to 12 characters -- including letters, numbers, hyphens, and dollar signs --
//  followed by a pound sign (#).
//  We also accept empty directory IDs, i.e., consisting solely of a # sign.
//  Empty directory IDs indicate that the user wants to use the implied directory ID, whatever it is.
CSInterpreter::Status
CSInterpreter::scanDirectoryId
(
    std::string* const      pDirectoryId
)
{
    //  Get directory ID
    std::string dirId;
    INDEX oldIndex = m_Index;
    while ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if (!( isalpha(ch) || isdigit(ch) || (ch == '-') || (ch == '$') ))
            break;
        dirId += ch;
        ++m_Index;
    }

    //  If not terminated by #, it's not a directory ID.
    if ( atEnd() || (m_Statement[m_Index] != '#') )
    {
        m_Index = oldIndex;
        return CSIST_NOT_FOUND;
    }
    ++m_Index;

    //  If it's > 12 characters, it's invalid
    if ( dirId.size() > 12 )
        return CSIST_INVALID_DIRECTORY_ID;

    *pDirectoryId = dirId;
    return CSIST_SUCCESSFUL;
}


//  scanField()
//
//  Scans a field (a comma-delimited list of subfields).
//  Call after scanning beyond leading whitespace.
//  Folds everything to uppercase.
//
//  format:
//      subfield [ '/' [ whitespace ] subfield ]*
//
//  Parameters:
//      pContainer:             vector of strings, where we store subsequent subfields
//      pSizeTemplate:          pointer to vector of COUNT objects, where the length of the vector
//                                  indicates the maximum number of subfields allowed, and the value
//                                  of each element indicates the maximum size allowed for the subfield
//                                  (zero indicating unlimited size)
CSInterpreter::Status
CSInterpreter::scanField
(
    VSTRING* const          pContainer,
    std::vector<COUNT>*     pSizeTemplate
)
{
    pContainer->clear();
    std::string subField;
    COUNT limit = 0;
    if ( pSizeTemplate )
        limit = (*pSizeTemplate)[0];
    Status sfStat = scanSubField( &subField, limit );
    if ( sfStat != CSIST_SUCCESSFUL )
        return sfStat;

    pContainer->push_back( subField );
    while ( ( !atEnd() ) && ( m_Statement[m_Index] == '/' ) )
    {
        if ( pSizeTemplate && ( subField.size() >= pSizeTemplate->size() ) )
            return CSIST_MAX_FIELDS_SUBFIELDS;

        //  'scan' separator
        ++m_Index;

        //  look for whitespace
        Status wsStat = scanWhiteSpace();
        if ( (wsStat != CSIST_SUCCESSFUL) && (wsStat != CSIST_NOT_FOUND) )
            return wsStat;

        //  look for next subfield
        COUNT limit = 0;
        if ( pSizeTemplate )
            limit = (*pSizeTemplate)[subField.size()];
        sfStat = scanSubField( &subField, limit );
        if ( (sfStat != CSIST_SUCCESSFUL) && (sfStat != CSIST_NOT_FOUND) )
            return sfStat;
        pContainer->push_back( subField );
    }

    return ( pContainer->size() > 0 ) ? CSIST_SUCCESSFUL : CSIST_NOT_FOUND;
}


//  scanFields()
//
//  Scan a set of comma-delimited fields.  Fold everything to uppercase.
//
//  format:
//      field [ ',' [ whitespace ] field ]*
//
//  Parameters:
//      pContainer:         A container of containers of strings.
//                              The major order represents the fields, minor represents the subfields.
//                              So, a,b/c,d would look like
//                                  [ [a], [b, c], [d] ]
//      pTemplate:          A container of containers which indicate:
//                              Number of fields allowed
//                              Number of subfields allowed
//                              Number of characters allowed in each subfield.
//                          So, assuming the above example, where a gets 6 characters, and b, c, and d get 12,
//                              we would have:
//                                  [ [6], [12, 12], [12] ]
//                          If not specified, there are no restrictions.
CSInterpreter::Status
CSInterpreter::scanFields
(
    std::vector<VSTRING>* const             pContainer,
    std::vector<std::vector<COUNT>>* const  pTemplate
)
{
    pContainer->clear();

    VSTRING subFieldContainer;
    std::vector<COUNT>* pSubTemplate = 0;
    if ( pTemplate )
        pSubTemplate = &(*pTemplate)[0];
    Status stat = scanField( &subFieldContainer, pSubTemplate );
    if ( stat != CSIST_SUCCESSFUL )
        return stat;
    pContainer->push_back( subFieldContainer );

    while ( ( !atEnd() ) && ( m_Statement[m_Index] == ',' ) )
    {
        if ( pTemplate && ( pContainer->size() >= pTemplate->size() ) )
            return CSIST_MAX_FIELDS_SUBFIELDS;

        ++m_Index;

        Status wsStat = scanWhiteSpace();
        if ( (wsStat != CSIST_SUCCESSFUL) && (wsStat != CSIST_NOT_FOUND) )
            return wsStat;

        subFieldContainer.clear();
        pSubTemplate = 0;
        if ( pTemplate )
            pSubTemplate = &(*pTemplate)[pContainer->size()];
        stat = scanField( &subFieldContainer, pSubTemplate );
        if ( stat == CSIST_NOT_FOUND )
            break;
        else if ( stat != CSIST_SUCCESSFUL )
            return stat;
        pContainer->push_back( subFieldContainer );
    }

    return ( pContainer->size() > 0 ) ? CSIST_SUCCESSFUL : CSIST_NOT_FOUND;
}


//  scanFileName()
//
//  Scans a file name
CSInterpreter::Status
CSInterpreter::scanFileName
(
    std::string* const      pFileName
)
{
    SuperString fname;
    while ( !atEnd() && !atSubFieldTerminator() && (m_Statement[m_Index] != '(') )
    {
        char ch = m_Statement[m_Index];
        if ( ch == '.' )
            break;

        if (!( isalpha(ch) || isdigit(ch) || (ch == '-') || (ch == '$') ))
        {
            //  Allow '@' in filenames for Exec caller
            if ( !m_pRunInfo->isExec() || ( ch != '@' ) )
                return CSIST_INVALID_FILE_NAME;
        }
        fname += ch;
        ++m_Index;
    }

    //  If it's > 12 characters or empty, it's invalid
    if ( (fname.size() > 12) || (fname.size() == 0) )
        return CSIST_INVALID_FILE_NAME;

    fname.foldToUpperCase();
    *pFileName = fname;
    return CSIST_SUCCESSFUL;
}


//  scanFileSpecification()
//
//  Scans a file spec, as follows:
//      keysSpec:
//          '/' [ readKey ] [ '/' [ writeKey ] ]
//      fileSpec:
//          basicFileSpec [ keysSpec ]
//
//  Important to note: We do NOT scan the period which often is taken as part of the file specification.
CSInterpreter::Status
CSInterpreter::scanFileSpecification
(
    FileSpecification* const    pFileSpec
)
{
    Status stat = scanBasicFileSpecification( pFileSpec );
    if ( stat == CSIST_SUCCESSFUL )
        return stat;

    stat = scanReadWriteKey( &pFileSpec->m_ReadKey );
    if ( stat == CSIST_NOT_FOUND )
        return CSIST_SUCCESSFUL;
    else if ( stat != CSIST_SUCCESSFUL )
        return stat;

    pFileSpec->m_ReadKey.foldToUpperCase();
    pFileSpec->m_KeysSpecified = true;
    stat = scanReadWriteKey( &pFileSpec->m_WriteKey );
    if ( stat == CSIST_SUCCESSFUL )
        pFileSpec->m_WriteKey.foldToUpperCase();
    else if ( stat != CSIST_NOT_FOUND )
        return stat;

    return CSIST_SUCCESSFUL;
}


//  scanLabel()
//
//  Scans a label token from the image's current index.
//  If a label is found, we return true and set m_Label appropriately.
//  If a label is not found, we return true, and leave m_Label blank.
//  If something *like* a label is found, but it is invalid, we return false.
CSInterpreter::Status
CSInterpreter::scanLabel()
{
    //  Parse out a token consisting of alphas and/or digits.  The first character must be alphabetic.
    //  If we don't get a token, then we don't have a label (but it is not an error).
    SuperString tempLabel;
    INDEX originalIndex = m_Index;
    while ( !atEnd()
            && ( isalpha( m_Statement[m_Index] )
                || ( (tempLabel.size() > 1) && (isdigit( m_Statement[m_Index] )) ) ) )
    {
        tempLabel += m_Statement[m_Index];
        ++m_Index;
    }

    //  If tempLabel is empty, we didn't find anything even resembling a label.
    //  Is the next character a colon?  If not, this is not a label.
    if ( atEnd() || (tempLabel.size() == 0) || (m_Statement[m_Index] != ':') )
    {
        m_Index = originalIndex;
        return CSIST_NOT_FOUND;
    }

    //  Increment the index to account for the colon, then see if the label is too long.
    ++m_Index;
    if ( tempLabel.size() > 6 )
        return CSIST_INVALID_LABEL;

    //  It's a good label - fold to uppercase, store it, and we're done.
    tempLabel.foldToUpperCase();
    m_Label = tempLabel;

    return CSIST_SUCCESSFUL;
}


//  scanLogicalOperator()
//
//  Looks for 'AND', 'OR', or 'XOR', followed by any terminator
inline CSInterpreter::Status
CSInterpreter::scanLogicalOperator
(
    SetcLogicalOp* const    pOperator
)
{
    if ( ( scanString( "AND", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pOperator = SLOP_AND;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "OR", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pOperator = SLOP_OR;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "XOR", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pOperator = SLOP_XOR;
        return CSIST_SUCCESSFUL;
    }

    return CSIST_NOT_FOUND;
}


//  scanMasterSpace()
//
//  Checks for the masterspace character at the current statement index.
inline CSInterpreter::Status
CSInterpreter::scanMasterSpace()
{
    if ( ( !atEnd() ) && ( m_Statement[m_Index] == '@' ) )
    {
        ++m_Index;
        return CSIST_SUCCESSFUL;
    }

    return CSIST_NOT_FOUND;
}


//  scanOctalInteger()
//
//  Scans an integer value comprised of octal digits, up to the first-encountered subfield terminator
//
//  Parameters:
//      pValue:         where we store the binary value
//      requireLeadingZero: if true, we return NOT_FOUND if the first character is not '0'
//      maxDigits:          maximum number of digits we accept - if we scan one digit beyond this
//                              limit, we return CSIST_ERROR
//
//  Returns:
//      CSIST_SUCCESSFUL:      if at least one octal digit was found and converted,
//                              and no non-octal digits were detected before the terminator was found
//      CSIST_NOT_FOUND:       if the first digit is not an octal digit,
//                              and if requireLeadingZero is true, if it is not '0'
//      CSIST_SYNTAX_ERROR:    if we detect a non-octal digit *after* an octal digit,
//                              and before a subfield terminator
CSInterpreter::Status
CSInterpreter::scanOctalInteger
(
    COUNT64* const  pValue,
    const bool      requireLeadingZero,
    const COUNT     maxDigits
)
{
    COUNT digits = 0;
    while ( !atSubFieldTerminator() )
    {
        char ch = m_Statement[m_Index];
        bool isoctal = ( ch >= '0' ) && ( ch <= '7' );
        if ( digits == 0 )
        {
            if ( !isoctal )
                return CSIST_NOT_FOUND;
            if ( requireLeadingZero && ch != '0' )
                return CSIST_NOT_FOUND;
        }
        else if ( !isoctal || ( digits == maxDigits ) )
        {
            ++m_Index;
            return CSIST_SYNTAX_ERROR;
        }

        (*pValue) = (*pValue) * 8 + ( ch - '0' );
        ++digits;
        ++m_Index;
    }

    if ( digits == 0 )
        return CSIST_NOT_FOUND;
    return CSIST_SUCCESSFUL;
}


//  scanOptions()
//
//  Scans alphabetical string of options.
//  There are a couple of commands which have non-standard options: @RUN and @START.
//  So, the caller must determine which characters are, or are not, allowed immediately
//  preceding and following this string.  We merely verify the alphabetic characters, and
//  generate the resulting bit mask.
//
//  If allowMask is provided, it is a mask of OPTB_x bits representing options which are
//  allowed.  Any option not in the mask will be disallowed.
CSInterpreter::Status
CSInterpreter::scanOptions
(
    UINT32* const       pMask,
    const UINT32        allowMask
)
{
    *pMask = 0;
    while ( !atEnd() && isalpha( m_Statement[m_Index] ) )
    {
        //  Get next potential option character from the statement
        char ch = m_Statement[m_Index];
        if ( islower(ch) )
            ch = toupper( ch );

        //  Create a bit in the position appropriate to the selected option
        COUNT shift = 'Z' - ch;
        UINT32 bit = 0x01 << shift;

        //  Make sure the option is allowed by ANDing the bit with the allowMask.
        //  If we fail, the caller has m_Index to know which option caused trouble.
        if ( (bit & allowMask) == 0 )
            return CSIST_INVALID_OPTION;

        *pMask = (*pMask) | (0x01 << shift);
        ++m_Index;
    }

    return (*pMask == 0 ) ? CSIST_NOT_FOUND : CSIST_SUCCESSFUL;
}


//  scanPartialWord()
//
//  Looks for 'T1', 'T2', 'S2', 'S3', or 'S4' followed by any terminator
CSInterpreter::Status
CSInterpreter::scanPartialWord
(
    SetcPartialWord* const      pPartialWord
)
{
    if ( ( scanString( "T1", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pPartialWord = SPAW_T1;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "T2", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pPartialWord = SPAW_T2;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "S2", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pPartialWord = SPAW_S2;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "S3", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pPartialWord = SPAW_S3;
        return CSIST_SUCCESSFUL;
    }

    if ( ( scanString( "S4", false ) == CSIST_SUCCESSFUL ) && atSubFieldTerminator() )
    {
        *pPartialWord = SPAW_S4;
        return CSIST_SUCCESSFUL;
    }

    return CSIST_NOT_FOUND;
}


//  scanPriority()
//
//  Scans for a single alphabetic character, which will be folded to uppercase if found.
CSInterpreter::Status
CSInterpreter::scanPriority
(
    char* const         pPriority
)
{
    if ( atEnd() )
        return CSIST_NOT_FOUND;
    char ch = m_Statement[m_Index];
    if ( isalpha( ch ) )
    {
        *pPriority = toupper( ch );
        ++m_Index;
        return CSIST_SUCCESSFUL;
    }

    return CSIST_NOT_FOUND;
}


//  scanQualifier()
//
//  Qualifiers consist of 1 to 12 characters -- including letters, numbers, hyphens, and dollar signs --
//  usually followed by an asterisk (*).
//  We also accept empty qualifiers, i.e., consisting solely of an asterisk.
//  Empty qualifiers indicate that the user wants to use the implied directory ID, whatever it is.
//
//  Optionally, we can be directed NOT to look for an asterisk, in which case any character not allowed
//  as a qualifier name, terminates the scan, and an empty qualifier is considered NOT_FOUND.
//
//  Normal format (scanAsterisk is true):
//      [ qualifier ] '*'
CSInterpreter::Status
CSInterpreter::scanQualifier
(
    std::string* const  pQualifier,
    const bool          scanAsterisk
)
{
    //  Get qualifier
    SuperString qual;
    INDEX oldIndex = m_Index;
    while ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if (!( isalpha(ch) || isdigit(ch) || (ch == '-') || (ch == '$') ))
            break;
        qual += ch;
        ++m_Index;
    }

    if ( scanAsterisk )
    {
        //  We're looking for an asterisk.
        //  If the given qual is not terminated by *, it's not a qualifier.
        if ( atEnd() || (m_Statement[m_Index] != '*') )
        {
            m_Index = oldIndex;
            return CSIST_NOT_FOUND;
        }
        ++m_Index;
    }
    else
    {
        //  Not looking for an asterisk - if the qual is empty, it's not a qualifier.
        if ( qual.size() == 0 )
        {
            m_Index = oldIndex;
            return CSIST_NOT_FOUND;
        }
    }

    //  If the given qual is > 12 characters, it's invalid
    if ( qual.size() > 12 )
        return CSIST_INVALID_QUALIFIER;

    qual.foldToUpperCase();
    *pQualifier = qual;
    return CSIST_SUCCESSFUL;
}


//  scanReadWriteKey()
//
//  Scans a read or write key from the index location.
//  Such a key will be preceeded by a forward-slash, and will contain 0 to 6 characters.
CSInterpreter::Status
CSInterpreter::scanReadWriteKey
(
    std::string* const      pKey
)
{
    //  Must begin with a /
    if ( atEnd() || ( m_Statement[m_Index] != '/' ) )
        return CSIST_NOT_FOUND;
    ++m_Index;

    SuperString key;
    while ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if ( ch == ' ' || ch == '.' || ch == ',' || ch == '/' || ch == ';')
            break;
        key += ch;
        ++m_Index;
    }

    key.foldToUpperCase();
    if ( (key.size() > 0) && (!Exec::isValidRWKey( key )) )
        return CSIST_INVALID_KEY;

    *pKey = key;
    return CSIST_SUCCESSFUL;
}


//  scanRelativeFileCycle()
//
//  Scans a relative file cycle from the image's current index.
//  Format is:
//      '(0)'
//      | '(+1)'
//      | '(-' n [ n ] ')'   . where 1 < nn < 32
CSInterpreter::Status
CSInterpreter::scanRelativeFileCycle
(
    INT16* const        pCycle
)
{
    if ( atEnd() )
        return CSIST_NOT_FOUND;

    if ( scanString( "(0)" ) == CSIST_SUCCESSFUL )
    {
        *pCycle = 0;
        return CSIST_SUCCESSFUL;
    }

    if ( scanString( "(+1)" ) == CSIST_SUCCESSFUL )
    {
        *pCycle = 1;
        return CSIST_SUCCESSFUL;
    }

    if ( scanString( "(-" ) != CSIST_SUCCESSFUL )
        return CSIST_NOT_FOUND;

    UINT32 cycle = 0;
    if ( atEnd() )
        return CSIST_INVALID_CYCLE;
    char ch = m_Statement[m_Index];
    if ( !isdigit( ch ) )
        return CSIST_INVALID_CYCLE;
    ++m_Index;
    cycle = ch - '0';

    if ( atEnd() )
        return CSIST_INVALID_CYCLE;
    if ( m_Statement[m_Index] == ')' )
        ++m_Index;
    else
    {
        ch = m_Statement[m_Index];
        if ( !isdigit( ch ) )
            return CSIST_INVALID_CYCLE;
        ++m_Index;
        cycle = cycle * 10 + (ch - '0');

        if ( atEnd() || ( m_Statement[m_Index] != ')' ) )
            return CSIST_INVALID_CYCLE;
        ++m_Index;
    }

    if ( (cycle == 0) || (cycle > 31) )
        return CSIST_INVALID_CYCLE;

    *pCycle = static_cast<INT16>(0 - cycle);
    return CSIST_SUCCESSFUL;
}


//  scanRemainder()
//
//  Scans the remainder of the image following the masterspace and optional label,
//  presuming it is a control statement with a known command.
//  Call here *after* scanning beyond any leading whitespace.
CSInterpreter::Status
CSInterpreter::scanRemainder()
{
    //  Get command, then check it for known values.
    SuperString command;
    while ( !atSubFieldTerminator() )
        command += m_Statement[m_Index++];

    //  If we have no command, and nothing left except whitespace, this is a label (or empty) statement.
    //  If there *is* something besides whitespace, don't error; just assume it's something other than
    //  a control statement.
    if ( command.size() == 0 )
    {
        Status stat = scanWhiteSpace( m_AllowContinuation );
        if ( ( stat != CSIST_SUCCESSFUL ) && ( stat != CSIST_NOT_FOUND ) )
            return stat;

        if ( atEnd() )
        {
            if ( m_Label.size() > 0 )
                m_Command = CMD_LABEL;
            else
                m_Command = CMD_EMPTY;
            return CSIST_SUCCESSFUL;
        }

        return CSIST_NOT_FOUND;
    }

    //  See if we recognize the command
    command.foldToUpperCase();
    if ( command.compare( "ASG" ) == 0 )
        return scanRemainderAsg();
    else if ( command.compare( "CAT" ) == 0 )
        return scanRemainderCat();
    else if ( command.compare( "FIN" ) == 0 )
        return scanRemainderFin();
    else if ( command.compare( "FREE" ) == 0 )
        return scanRemainderFree();
    else if ( command.compare( "LOG" ) == 0 )
        return scanRemainderLog();
    else if ( command.compare( "MSG" ) == 0 )
        return scanRemainderMsg();
    else if ( command.compare( "QUAL" ) == 0 )
        return scanRemainderQual();
    else if ( command.compare( "RUN" ) == 0 )
        return scanRemainderRun();
    else if ( command.compare( "SETC" ) == 0 )
        return scanRemainderSetc();
    else if ( command.compare( "USE" ) == 0 )
        return scanRemainderUse();
    else if ( m_pExec->getConfiguration().getBoolValue( "EXECL" ) )
    {
        if ( command.compare( "DIR" ) == 0 )
            return scanRemainderDir();
        else if ( command.compare( "RINFO" ) == 0 )
            return scanRemainderRinfo();
    }

    return CSIST_NOT_FOUND;
}


//  scanRemainderAsg()
//
//  We interpret up to and including the file specification.
//  Everything else is converted into a list of fields, with each field stored as a list of subfields.
//
//      format:
//          [ ',' options whiteSpace ]  fileSpecification [ '.' ]  additionalFields
CSInterpreter::Status
CSInterpreter::scanRemainderAsg()
{
    m_Command = CMD_ASG;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        UINT32 optionMask = OPTB_I | OPTB_T | OPTB_Z;
        optionMask |= OPTB_B | OPTB_C | OPTB_G | OPTB_P | OPTB_R | OPTB_S | OPTB_U | OPTB_V | OPTB_W;
        optionMask |= OPTB_A | OPTB_D | OPTB_E | OPTB_K | OPTB_M | OPTB_Q | OPTB_R | OPTB_X | OPTB_Y;
        Status stat = scanOptions( &m_Options, optionMask );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    //  Scan file specification
    if ( atEnd() )
        return CSIST_MISSING_FILE_SPECIFICATION;

    stat = scanFileSpecification( &m_FileSpec1 );
    if ( stat != CSIST_SUCCESSFUL )
        return stat;

    //  Optional period after the filespec
    if ( !atEnd() )
    {
        if ( m_Statement[m_Index] == '.' )
            ++m_Index;
    }

    stat = scanAdditionalFields();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    if ( !atEnd() )
        return CSIST_SYNTAX_ERROR;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderCat()
//
//  We interpret up to and including the file specification.
//  Everything else is converted into a list of fields, with each field stored as a list of subfields.
//
//      format:
//          [ ',' options whiteSpace ]  fileSpecification [ '.' ]  additionalFields
CSInterpreter::Status
CSInterpreter::scanRemainderCat()
{
    m_Command = CMD_CAT;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        UINT32 optionMask = OPTB_B | OPTB_E | OPTB_G | OPTB_H | OPTB_J | OPTB_L
                            | OPTB_M | OPTB_O | OPTB_P | OPTB_R | OPTB_S | OPTB_V | OPTB_W | OPTB_Z;
        Status stat = scanOptions( &m_Options, optionMask );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    //  Scan file specification
    if ( atEnd() )
        return CSIST_MISSING_FILE_SPECIFICATION;

    stat = scanFileSpecification( &m_FileSpec1 );
    if ( stat != CSIST_SUCCESSFUL )
        return stat;

    //  Optional period after the filespec
    if ( !atEnd() )
    {
        if ( m_Statement[m_Index] == '.' )
            ++m_Index;
    }

    stat = scanAdditionalFields();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    if ( !atEnd() )
        return CSIST_SYNTAX_ERROR;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderDir()
//
//  Undocumented/Exented command to print MFD sectors and/or other information
//      @DIR,[option] [fileSpec | directorySectorAddress]
//      Options:
//          O   dump contents of given directory sector address in octal
//          A   dump contents of given directory sector address in ascii
//          F   dump contents of given directory sector address in fieldata
//          S   print search item addresses associated with the given q*f
//          L   print lead item addresses associated with the given q*f
//          M   print main item addresses associated with the given q*f(cyc)
CSInterpreter::Status
CSInterpreter::scanRemainderDir()
{
    m_Command = CMD_DIR;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    UINT32 dumpOptions = OPTB_O | OPTB_A | OPTB_F;
    UINT32 listOptions = OPTB_S | OPTB_L | OPTB_M;

    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        Status stat = scanOptions( &m_Options, dumpOptions | listOptions );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  We've filtered out illegal options; now we have to filter combinations
    if (  m_Options != 0 )
    {
        if ( (m_Options & OPTB_S) && (m_Options != OPTB_S) )
            return CSIST_INVALID_OPTION_COMBINATION;
        if ( (m_Options & OPTB_L) && (m_Options != OPTB_L) )
            return CSIST_INVALID_OPTION_COMBINATION;
        if ( (m_Options & OPTB_M) && (m_Options != OPTB_M) )
            return CSIST_INVALID_OPTION_COMBINATION;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    if ( m_Options & dumpOptions )
    {
        //  O, A, or F options?  Scan an octal number.  If not found, default to sector 0 of LDAT 1.
        stat = scanOctalInteger( &m_Value, false, 10 );
        if ( stat == CSIST_NOT_FOUND )
            m_Value = 0001000000;
        else if ( stat != CSIST_SUCCESSFUL )
            return stat;
    }
    else if ( m_Options & listOptions )
    {
        //  S, L, M, D or R - scan a file specification
        stat = scanFileSpecification( &m_FileSpec1 );
        if ( stat == CSIST_NOT_FOUND )
            return CSIST_MISSING_FILE_SPECIFICATION;
        else if ( stat != CSIST_SUCCESSFUL )
            return stat;
    }

    return CSIST_SUCCESSFUL;
}


//  scanRemainderFin()
//
//  No options, arguments, nothing.
CSInterpreter::Status
CSInterpreter::scanRemainderFin()
{
    m_Command = CMD_FIN;

    //  Look for options we do not accept
    if ( (!atEnd()) && (m_Statement[m_Index] == ',') )
    {
        Status stat = scanOptions( &m_Options, 0 );
        if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
            return stat;
    }

    Status stat = scanWhiteSpace( false );
    if ( ( stat != CSIST_SUCCESSFUL ) && ( stat != CSIST_NOT_FOUND ) )
        return stat;
    if ( !atEnd() )
        return CSIST_EXTRANEOUS_TEXT;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderFree()
//
//  We interpret up to and including the file specification.
//  Everything else is converted into a list of fields, with each field stored as a list of subfields.
//
//      format:
//          [ ',' options whiteSpace ]  basicFileSpecification [ '.' ]
CSInterpreter::Status
CSInterpreter::scanRemainderFree()
{
    m_Command = CMD_FREE;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        UINT32 optionMask = OPTB_A | OPTB_B | OPTB_D | OPTB_I | OPTB_R | OPTB_S | OPTB_X;
        Status stat = scanOptions( &m_Options, optionMask );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    //  Scan file specification
    if ( atEnd() )
        return CSIST_MISSING_FILE_SPECIFICATION;

    stat = scanBasicFileSpecification( &m_FileSpec1 );
    if ( stat != CSIST_SUCCESSFUL )
        return stat;

    //  Optional period after the filespec
    if ( !atEnd() )
    {
        if ( m_Statement[m_Index] == '.' )
            ++m_Index;
    }

    stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    if ( !atEnd() )
        return CSIST_SYNTAX_ERROR;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderLog()
//
//  No options allowed, so a comma after the command is a syntax error.
//  The message to be logged cannot begin with a comma or a slash (don't know why, really).
//  It is limited to 132 characters.
CSInterpreter::Status
CSInterpreter::scanRemainderLog()
{
    m_Command = CMD_LOG;

    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    while ( !atEnd() && (m_Text.size() < 132) )
        m_Text += m_Statement[m_Index++];
    m_Text.foldToUpperCase();

    return CSIST_SUCCESSFUL;
}


//  scanRemainderMsg()
//
//  Various options are allowed.
//  The message cannot begin with a comma or a slash (don't know why, really).
//  It is limited to 60 characters.
CSInterpreter::Status
CSInterpreter::scanRemainderMsg()
{
    m_Command = CMD_MSG;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        UINT32 optionMask = OPTB_C | OPTB_H | OPTB_I | OPTB_N | OPTB_S | OPTB_W;
        Status stat = scanOptions( &m_Options, optionMask );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    //  Start picking up message from first non-whitespace character.
    //  For some reason, the message cannot start with comma or slash.
    //  Per ECL guide, message is folded to uppercase.
    if ( !atEnd() )
    {
        char ch = m_Statement[m_Index];
        if ( (ch == ',') || (ch == '/') )
            return CSIST_SYNTAX_ERROR;
    }

    while ( !atEnd() && (m_Text.size() < 60) )
        m_Text += m_Statement[m_Index++];
    m_Text.foldToUpperCase();
    return CSIST_SUCCESSFUL;
}


//  scanRemainderQual()
//
//  Scans the options and arguments for an @QUAL statement.
//  The qualifer (if specified) is stored in m_Qualifier
//
//      format:
//          [ ',' options ]
//          [ ',' options whiteSpace ]  qualifier
CSInterpreter::Status
CSInterpreter::scanRemainderQual()
{
    m_Command = CMD_QUAL;

    //  If the next character is a comma, scan options.
    //  Since this is @QUAL, we don't do much validation here...
    //  Facilities will do all evaluation later, during execution.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        Status stat = scanOptions( &m_Options );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    //  If there's anything left, it is of the form:
    //      [ qualifier ]
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    stat = scanQualifier( &m_Qualifier, false );
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;

    //  make sure nothing is left over
    stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && (stat != CSIST_NOT_FOUND) )
        return stat;
    if ( !atEnd() )
        return CSIST_EXTRANEOUS_TEXT;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderRinfo()
//
//  Undocumented/Exented command to print RunInfo information to PRINT$
//      @RINFO
CSInterpreter::Status
    CSInterpreter::scanRemainderRinfo()
{
    //  Don't do much sanity here, since this is undocumented
    m_Command = CMD_RINFO;
    return CSIST_SUCCESSFUL;
}


//  scanRemainderRun()
//
//  Scans all the stuff for an @RUN statement
//      @RUN,scheduling-priority/options/processor-dispatching-priority
//          run-id,account/user-id,project-id,run-time/deadline,pages/cards,start-time
//  All subfields are optional, and field/subfield separators are optional wherever they
//  are not necessary for delineating field/subfield position.
CSInterpreter::Status
CSInterpreter::scanRemainderRun()
{
    m_Command = CMD_RUN;

    //  If the next character is a comma, scan for options and priorities.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        Status stat = scanRunOptionsAndPriorities();
        if ( stat != CSIST_SUCCESSFUL && stat != CSIST_NOT_FOUND )
            return stat;
    }

    //  Now start working on the main fields.
    //  Pull arguments from m_Statement...
    Status stat = scanWhiteSpace( false );
    if ( ( stat != CSIST_SUCCESSFUL ) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    //  Create template for scanning the operand fields.
    //  For Batch:  [ [6], [12, 12], [12], [12, 5], [6, 6], [5] ]
    //  For Demand: [ [6], [12, 12], [12] ]
    std::vector<std::vector<COUNT>> limits;
    std::vector<COUNT> subLimits;

    subLimits.push_back(6);
    limits.push_back( subLimits );

    subLimits.clear();
    subLimits.push_back( 12 );
    subLimits.push_back( 12 );
    limits.push_back( subLimits );

    subLimits.clear();
    subLimits.push_back( 12 );
    limits.push_back( subLimits );

    if ( !m_DemandFlag )
    {
        subLimits.clear();
        subLimits.push_back( 12 );
        subLimits.push_back( 5 );
        limits.push_back( subLimits );

        subLimits.clear();
        subLimits.push_back( 6 );
        subLimits.push_back( 6 );
        limits.push_back( subLimits );

        subLimits.clear();
        subLimits.push_back( 5 );
        limits.push_back( subLimits );
    }

    //  Segregate the arguments string into fields
    std::vector<VSTRING> fields;
    stat = scanFields( &fields, &limits );
    if ( stat == CSIST_SUCCESSFUL )
    {
        if ( fields.size() > 6 )
            return CSIST_SYNTAX_ERROR;

        //  Check run-id
        if ( (fields.size() >= 1) && (fields[0].size() >= 1) )
        {
            if ( !Exec::isValidRunId( fields[0][0] ) )
                return CSIST_INVALID_RUN_ID;
            m_RunId = fields[0][0];
        }

        //  Check account-id/user-id
        if ( fields.size() >= 2 )
        {
            if (( fields[1].size() >= 1 ) && (fields[1][0].size() > 0))
            {
                if ( !Exec::isValidAccountId( fields[1][0] ) )
                    return CSIST_INVALID_ACCOUNT_ID;
                m_AccountId = fields[1][0];
            }
            if ( fields[1].size() >= 2 )
            {
                if ( !Exec::isValidUserId( fields[1][1] ) )
                    return CSIST_INVALID_USER_ID;
                m_UserId = fields[1][1];
            }
        }

        //  Check project-id
        if ( (fields.size() >= 3) && (fields[2].size() >= 1) )
        {
            if ( !Exec::isValidProjectId( fields[2][0] ) )
                return CSIST_INVALID_PROJECT_ID;
            m_ProjectId = fields[2][0];
        }

        //TODO:BATCH more to do
        //  run-time/deadline,pages/cards/starttime
        //  run-time format:    [ 'S' ] n*      (minutes up to 12 digits, or seconds up to 11 digits)
        //  deadline format:    [ 'D' ] n*      ([D] hhmm)
        //  pages format:       n*              ( up to 6 digits )
        //  cards format:       n*              ( up to 6 digits )
        //  start-time format:  [ 'D' ] n*      ([D] hhmm)
    }
    else if ( stat == CSIST_CONTINUED )
        return CSIST_ILLEGAL_CONTINUATION;
    else if ( stat != CSIST_NOT_FOUND )
        return stat;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderSetc()
//
//  Format:
//      valid_option:   [ 'A' | 'I' | 'N' | 'P' ]
//      options:        valid_option*
//      logical_op:     [ 'AND' | 'OR' | 'XOR' ]
//      octal_value:    octal_digit [ octal_digit [ octal_digit [ octal_digit ]]]
//      partial_word:   [ 'T1' | 'T2' | 'S2' | 'S3' | 'S4' ]
//      statement:
//          [label] '@SETC' [ ',' [ options ]] whitespace [ logical_op '/' ] octal_value [ '/' partial_word ]
CSInterpreter::Status
CSInterpreter::scanRemainderSetc()
{
    m_Command = CMD_SETC;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        UINT32 optionMask = OPTB_A | OPTB_I | OPTB_N | OPTB_P;
        Status stat = scanOptions( &m_Options, optionMask );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;

        //  A and I are mutually exclusive, as are N and P
        if ( (m_Options & OPTB_A) && (m_Options & OPTB_I) )
            return CSIST_INVALID_OPTION_COMBINATION;
        if ( (m_Options & OPTB_N) && (m_Options & OPTB_P) )
            return CSIST_INVALID_OPTION_COMBINATION;
    }

    //  Skip whitespace
    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    //  Look for logical op - if found, it must be immediately followed by a slash character.
    stat = scanLogicalOperator( &m_LogicalOp );
    if ( stat == CSIST_SUCCESSFUL )
    {
        stat = scanString( "/" );
        if ( stat != CSIST_SUCCESSFUL )
            return CSIST_SYNTAX_ERROR;
    }

    //  Get required octal value
    stat = scanOctalInteger( &m_Value, false, 4 );
    if ( stat != CSIST_SUCCESSFUL )
        return CSIST_SYNTAX_ERROR;

    //  Look for additional slash, which if found, must be immediately followed by a partial-word name.
    stat = scanString( "/" );
    if ( stat == CSIST_SUCCESSFUL )
    {
        stat = scanPartialWord( &m_PartialWord );
        if ( stat != CSIST_SUCCESSFUL )
            return CSIST_SYNTAX_ERROR;
    }

    //  Skip remaining whitespace
    stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    //  Make sure nothing is left
    if ( !atEnd() )
        return CSIST_EXTRANEOUS_TEXT;

    return CSIST_SUCCESSFUL;
}


//  scanRemainderUse()
//
//  Scans the options and arguments for an @USE statement
CSInterpreter::Status
CSInterpreter::scanRemainderUse()
{
    m_Command = CMD_USE;

    //  If the next character is a comma, scan options.
    //  They should be followed immediately by either a blank, or end-of-statement.
    if ( !atEnd() && m_Statement[m_Index] == ',' )
    {
        ++m_Index;
        Status stat = scanOptions( &m_Options, OPTB_I );
        if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
            return stat;
    }

    Status stat = scanWhiteSpace();
    if ( (stat != CSIST_SUCCESSFUL) && ( stat != CSIST_NOT_FOUND ) )
        return stat;

    //  Scan the first filespec - it should be only an internal filename.
    if ( atEnd() )
        return CSIST_MISSING_FILE_SPECIFICATION;

    stat = scanFileSpecification( &m_FileSpec1 );
    if ( stat == CSIST_NOT_FOUND )
        return CSIST_MISSING_FILE_SPECIFICATION;
    else if ( stat != CSIST_SUCCESSFUL )
        return stat;
    if ( !m_FileSpec1.isFileNameOnly() )
        return CSIST_INVALID_INTERNAL_NAME;

    //  Skip optional period
    if ( !atEnd() && m_Statement[m_Index] == '.' )
        ++m_Index;

    //  If there's a space (or nothing left), the second filespec is missing.
    if ( atEnd() || m_Statement[m_Index] == ASCII_SPACE )
        return CSIST_MISSING_FILE_SPECIFICATION;

    //  If there's anything but a comma, we have a syntax error.
    if ( m_Statement[m_Index] != ',' )
        return CSIST_SYNTAX_ERROR;
    ++m_Index;

    //  Skip optional whitespace
    scanWhiteSpace();

    //  Scan the second filespec - it can contain a directory ID, qualifier, filename, and cycle.
    //  No read or write keys, and the filename is required (the other fields are optional).
    stat = scanFileSpecification( &m_FileSpec2 );
    if ( stat == CSIST_NOT_FOUND )
        return CSIST_MISSING_FILE_SPECIFICATION;
    else if ( stat != CSIST_SUCCESSFUL )
        return stat;
    if ( m_FileSpec2.m_KeysSpecified )
        return CSIST_KEYS_NOT_ALLOWED;

    //  If there's a trailing '.', eat it silently.
    if ( !atEnd() && (m_Statement[m_Index] == '.') )
        ++m_Index;

    //  We should now be at the end, or at whitespace
    if ( !atEnd() && ( scanWhiteSpace() != CSIST_SUCCESSFUL ) )
        return CSIST_SYNTAX_ERROR;

    return CSIST_SUCCESSFUL;
}


//  scanRunOptionsAndPriorities()
//
//  Call if @RUN parseing finds a comma immediately following @RUN.
//  Scans the options and priorities field for the RUN statement:
//      [ scheduling-priority ] '/' [ options ] '/' [ processor-dispatching-priority ]
//  This sequence must be terminated either by a space, or by end-of-image.
//  There is no CSIST_NOT_FOUND condition - the caller found the comma, so by default
//  we *do* exist... we may just be 'empty'.
CSInterpreter::Status
CSInterpreter::scanRunOptionsAndPriorities()
{
    //  If we've reached the end of the field, return to the caller
    if ( atEnd() || m_Statement[m_Index] == ' ' )
        return CSIST_SUCCESSFUL;

    //  Scan the scheduling priority (if it's there)
    Status stat = scanPriority( &m_SchedulingPriority );
    if ( stat == CSIST_SYNTAX_ERROR )
        return stat;

    //  If we've reached the end of the field, return to the caller
    if ( atEnd() || m_Statement[m_Index] == ' ' )
        return CSIST_SUCCESSFUL;

    //  Next character must be a slash
    if ( m_Statement[m_Index] != '/' )
        return CSIST_SYNTAX_ERROR;
    ++m_Index;

    //  Scan the options for this image
    UINT32 allowMask = OPTB_B | OPTB_D | OPTB_E | OPTB_F | OPTB_G | OPTB_H | OPTB_I | OPTB_J
                                | OPTB_K | OPTB_L | OPTB_M | OPTB_N | OPTB_O | OPTB_Y | OPTB_W;
    stat = scanOptions( &m_Options, allowMask );
    if ( stat == CSIST_ILLEGAL_OPTION )
        return stat;

    //  If we've reached the end of the field, return to the caller
    if ( atEnd() || m_Statement[m_Index] == ' ' )
        return CSIST_SUCCESSFUL;

    //  Next character must be a slash
    if ( m_Statement[m_Index] != '/' )
        return CSIST_SYNTAX_ERROR;
    ++m_Index;

    //  Scan the dispatching priority (if it's there)
    stat = scanPriority( &m_ProcessorDispatchingPriority );
    if ( stat == CSIST_SYNTAX_ERROR )
        return stat;

    //  If we've reached the end of the field, return to the caller
    if ( atEnd() || m_Statement[m_Index] == ' ' )
        return CSIST_SUCCESSFUL;

    //  If we're still here, there's extraneous garbage which we won't accept.
    return CSIST_SYNTAX_ERROR;
}


//  scanString()
//
//  Scans a specific string from the index location of m_Statement, if it is there.
CSInterpreter::Status
CSInterpreter::scanString
(
    const std::string&      testString,
    const bool              caseSensitive
)
{
    INDEX oldIndex = m_Index;
    INDEX tsx = 0;
    while ( !atEnd() && (tsx < testString.size()) )
    {
        bool match = false;
        if ( caseSensitive && ( m_Statement[m_Index] == testString[tsx] ) )
            match = true;
        else if ( !caseSensitive && ( toupper( m_Statement[m_Index] ) == toupper( testString[tsx] ) ) )
            match = true;

        if ( !match )
        {
            m_Index = oldIndex;
            return CSIST_NOT_FOUND;
        }

        ++m_Index;
        ++tsx;
    }

    if ( tsx < testString.size() )
    {
        m_Index = oldIndex;
        return CSIST_NOT_FOUND;
    }

    return CSIST_SUCCESSFUL;
}


//  scanSubField()
//
//  Scans a subfield - call after scanning beyond whitespace.
//  Folds to uppercase.
//
//  Parameters:
//      pSubField:          pointer to string which we populate with the subfield
//      maxChars:           Maximum length permitted for subfield, zero (default) for no limit
CSInterpreter::Status
CSInterpreter::scanSubField
(
    std::string* const      pSubField,
    const COUNT             maxChars
)
{
    pSubField->clear();
    while ( !atSubFieldTerminator() )
    {
        if ( ( maxChars > 0 ) && ( pSubField->size() == maxChars ) )
            return CSIST_MAX_SUBFIELD_CHARS;
        char ch = m_Statement[m_Index++];
        if ( islower( ch ) )
            ch = toupper( ch );
        (*pSubField) += ch;
    }

    if ( pSubField->size() == 0 )
        return CSIST_NOT_FOUND;
    else if ( pSubField->size() <= 12 )
        return CSIST_SUCCESSFUL;
    else
        return CSIST_INVALID_SUBFIELD;
}


//  scanWhiteSpace()
//
//  Scans through any whitespace, from the current index.
//  Detects and handles continuation characters.  Also handles comments.
//
//  Parameters:
//      allowContinuation:      true to allow the continuation sentinel, else false
//
//  Returns:
//      CSIST_NOT_FOUND            if there is no whitespace
//      CSIST_SUCCESSFUL           if there is at least one character of whitespace
//                                  (including commentary or continuation sentinel)
//      CSIST_CONTINUED            we found a continuation character in the last image
//      CSIST_ILLEGAL_CONTINUATION if there is a misplaced or misused continuation sentinel
//      CSIST_SYNTAX_ERROR         for other strangeness
CSInterpreter::Status
CSInterpreter::scanWhiteSpace
(
    const bool          allowContinuation
)
{
    //  Scan beyond leading blanks
    bool blank = false;
    while ( ( !atEnd() ) && ( m_Statement[m_Index] == ASCII_SPACE ) )
    {
        blank = true;
        ++m_Index;
    }

    //  No more leading blanks.
    //  If we have at least one of them, and are now at a period-space or period-endofimage,
    //  this is a comment.  We *could* be in a strange situation where we have a comment on an
    //  image which is not the last image in the stack - that would be an error.
    if ( blank
        && ( !atEnd() ) && ( m_Statement[m_Index] == '.' )
        && (( m_Index == m_Statement.size() - 1 ) || ( m_Statement[m_Index + 1] == ASCII_SPACE )) )
    {
        INDEX imgx, colx;
        decomposeIndex( m_Index, &imgx, &colx );
        if ( imgx < m_Images.size() - 1 )
            return CSIST_SYNTAX_ERROR;

        //  Set index to the end, set comment flag, and we're done.
        m_Index = m_Statement.size();
        m_CommentFlag = true;
        return CSIST_SUCCESSFUL;
    }

    //  Are we at the end, or at something other than a continuation character?
    //  If so, we're done.
    if ( ( atEnd() ) || ( m_Statement[m_Index] != ';' ) )
        return blank ? CSIST_SUCCESSFUL : CSIST_NOT_FOUND;

    //  From here on, we have found a continuation character in the current image.
    //  The current image may or may not be the last image, so we don't know (yet)
    //  whether we have a complete statement.

    //  First, is the continuation character allowed?
    if ( !allowContinuation )
        return CSIST_ILLEGAL_CONTINUATION;

    //  Figure out the image/column breakdown for the current index, so we can see if this
    //  continuation character is in the last image - if so, set the continuation flag.
    bool continued = false;
    INDEX imgx, colx;   //  image and column indices for continuation character
    decomposeIndex( m_Index, &imgx, &colx );
    if ( imgx == m_Images.size() - 1 )
        continued = true;

    //  Move on to the next character after the continuation character, and ensure
    //  that all text *after* the continuation character, on the same image, is blank.
    ++m_Index;
    INDEX imgy, coly;   //  image and column indices for subsequent character
    decomposeIndex( m_Index, &imgy, &coly );
    while ( ( !atEnd() ) && ( imgx == imgy ) )
    {
        if ( m_Statement[m_Index] != ASCII_SPACE )
            return CSIST_ILLEGAL_CONTINUATION;
        ++m_Index;
        decomposeIndex( m_Index, &imgy, &coly );
    }

    //  If continued is true, we have nothing left to scan... return appropriately
    if ( continued )
        return CSIST_CONTINUED;

    //  We are not at the end of the statement...
    //  Therefore we are at the first column of the next image in the list of images.
    //  The first column of any such image must NOT contain a masterspace character.
    if ( scanMasterSpace() == CSIST_SUCCESSFUL )
        return CSIST_ILLEGAL_CONTINUATION;

    //  Keep looking for whitespace...
    return scanWhiteSpace();
}



//  private and protected static methods

//  isValidSubfield()
//
//  Ensures that a given candidate subfield is less than or equal to 12 characters in length,
//  and contains only letters and digits.
inline bool
CSInterpreter::isValidSubfield
(
    const std::string&      subfieldStr
)
{
    if ( subfieldStr.size() > 12 )
        return false;
    for ( INDEX sx = 0; sx < subfieldStr.size(); ++sx )
    {
        if ( !isdigit( subfieldStr[sx] ) && !isalpha( subfieldStr[sx] ) )
            return false;
    }
    return true;
}



//  constructors, destructors

CSInterpreter::CSInterpreter
(
    Exec* const             pExec,
    Activity* const         pActivity,          //  pointer to requesting Activity
    SecurityContext* const  pSecurityContext,   //  pointer to security context for this command
    RunInfo* const          pRunInfo            //  pointer to RunInfo object of interest
)
:m_pConsMgr( dynamic_cast<ConsoleManager*>( pExec->getManager( Exec::MID_CONSOLE_MANAGER ) ) ),
m_pExec( pExec ),
m_pFacMgr( dynamic_cast<FacilitiesManager*>( pExec->getManager( Exec::MID_FACILITIES_MANAGER ) ) ),
m_pMfdMgr( dynamic_cast<MFDManager*>( pExec->getManager( Exec::MID_MFD_MANAGER ) ) ),
m_pActivity( pActivity ),
m_pRunInfo( pRunInfo ),
m_pSecurityContext( pSecurityContext )
{
    initialize();
}


CSInterpreter::~CSInterpreter()
{
}



//  public methods

//  decomposeIndex()
//
//  Breaks a statement index into an image and image column index for diagnostics.
//  Note that column index is zero-biased.
//
//  Returns:
//      true if the index is valid, else false
bool
CSInterpreter::decomposeIndex
(
    const INDEX     index,
    INDEX* const    pImageIndex,
    INDEX* const    pColumnIndex
)
{
    assert( m_Images.size() > 0 );

    *pColumnIndex = index;
    *pImageIndex = 0;

    LCITSTRING itimage = m_Images.begin();
    while ( (*pColumnIndex) >= itimage->size() )
    {
        COUNT currentSize = itimage->size();
        ++itimage;
        if ( itimage == m_Images.end() )
            return false;

        (*pColumnIndex) -= currentSize;
        (*pImageIndex)++;
    }

    return true;
}


//  executeStatement()
//
//  Executes a previously-interpreted statement.
//
//  Parameters:
//      pActivity:          pointer to the activity requesting the execution
CSInterpreter::Status
CSInterpreter::executeStatement
(
    Activity* const     pActivity
)
{
    m_pActivity = pActivity;
    m_Status = CSIST_INVALID_COMMAND_CODE;

    if ( m_Command == CMD_LABEL )
        m_Status = CSIST_SUCCESSFUL;
    else if ( m_Command == CMD_ASG )
        m_Status = executeAsg();
    else if ( m_Command == CMD_CAT )
        m_Status = executeCat();
    else if ( m_Command == CMD_FIN )
        m_Status = executeFin();
    else if ( m_Command == CMD_FREE )
        m_Status = executeFree();
    else if ( m_Command == CMD_LOG )
        m_Status = executeLog();
    else if ( m_Command == CMD_MSG )
        m_Status = executeMsg();
    else if ( m_Command == CMD_QUAL )
        m_Status = executeQual();
    else if ( m_Command == CMD_RUN )
        m_Status = executeRun();
    else if ( m_Command == CMD_SETC )
        m_Status = executeSetc();
    else if ( m_Command == CMD_USE )
        m_Status = executeUse();
    else if ( m_pExec->getConfiguration().getBoolValue( "EXECL" ) )
    {
        if ( m_Command == CMD_DIR )
            m_Status = executeDir();
        else if ( m_Command == CMD_RINFO )
            m_Status = executeRinfo();
    }

    return m_Status;
}


//  interpretStatement()
//
//  Sets up the image stack given a single image, then calls interpret()
CSInterpreter::Status
CSInterpreter::interpretStatement
(
    const std::string&      image,
    const bool              demandFlag,
    const bool              allowContinuation,
    const bool              allowLabel
)
{
    initialize();

    m_AllowContinuation = allowContinuation;
    m_AllowLabel = allowLabel;
    m_DemandFlag = demandFlag;
    m_Images.push_back( image );
    m_Statement = image;

    return interpret();
}


//  interpretStatement()
//
//  Sets up the image stack and composite statement, then calls interpret().
CSInterpreter::Status
CSInterpreter::interpretStatementStack
(
    const LSTRING&          imageStack,
    const bool              demandFlag,
    const bool              allowContinuation,
    const bool              allowLabel
)
{
    initialize();

    m_AllowContinuation = allowContinuation;
    m_AllowLabel = allowLabel;
    m_DemandFlag = demandFlag;
    m_Images = imageStack;
    for ( LCITSTRING its = m_Images.begin(); its != m_Images.end(); ++its )
        m_Statement.append( *its );

    return interpret();
}


//  isErrorStatus()
//
//  Checks m_Status and possibly the fac result to see if the most recent
//  interpret or execute action resulted in an error which should cause
//  the run to go into error mode.
bool
CSInterpreter::isErrorStatus() const
{
    if ( m_Status == CSIST_SUCCESSFUL )
        return false;
    else if (m_Status == CSIST_FACILITIES_RESULT)
        return m_FacilitiesResult.containsErrorStatusCode();
    else
        return true;
}


//  postExecuteStatusToPrint()
//
//  For ControlMode calls - posts an appropriate message (which might include no message) to PRINT$
void
CSInterpreter::postExecuteStatusToPrint
(
    ControlModeRunInfo* const   pRunInfo
) const
{
    switch ( m_Status )
    {
    case CSIST_SUCCESSFUL:
        break;

    case CSIST_FACILITIES_RESULT:
        m_FacilitiesResult.postToPrint( pRunInfo );
        break;

    case CSIST_INTERRUPTED:
    case CSIST_INVALID_COMMAND_CODE:
        assert( false );

    case CSIST_INVALID_OPTION:
        pRunInfo->postToPrint( "INVALID OPTION" );
        break;

    case CSIST_INVALID_OPTION_COMBINATION:
        pRunInfo->postToPrint( "INVALID OPTION COMBINATION" );
        break;

    case CSIST_NOT_ALLOWED:
        pRunInfo->postToPrint( "STATEMENT NOT ALLOWED" );
        break;

    default:
        //  Things which should never happen are ignored
        break;
    }
}


//  postInterpretStatusToPrint()
//
//  For ControlMode calls - posts an appropriate message (which might include no message) to PRINT$
void
CSInterpreter::postInterpretStatusToPrint
(
    ControlModeRunInfo* const   pRunInfo
) const
{
    static const char* pDiagString;

    switch ( m_Status )
    {
    case CSIST_ILLEGAL_CONTINUATION:
        pDiagString = "ILLEGAL CONTINUATION";
        break;

    case CSIST_ILLEGAL_OPTION:
        pDiagString = "ILLEGAL OPTION";
        break;

    case CSIST_INVALID_ACCOUNT_ID:
        pDiagString = "INVALID ACCOUNT ID";
        break;

    case CSIST_INVALID_CYCLE:
        pDiagString = "INVALID F-CYCLE";
        break;

    case CSIST_INVALID_DIRECTORY_ID:
        pDiagString = "INVALID DIRECTORY ID";
        break;

    case CSIST_INVALID_FILE_NAME:
        pDiagString = "INVALID FILENAME";
        break;

    case CSIST_INVALID_INTERNAL_NAME:
        pDiagString = "INVALID INTERNAL FILENAME";
        break;

    case CSIST_INVALID_KEY:
        pDiagString = "INVALID READ/WRITE KEY";
        break;

    case CSIST_INVALID_LABEL:
        pDiagString = "INVALID LABEL";
        break;

    case CSIST_INVALID_OPTION:
        pDiagString = "INVALID OPTION";
        break;

    case CSIST_INVALID_OPTION_COMBINATION:
        pDiagString = "INVALID OPTION COMBINATION";
        break;

    case CSIST_INVALID_PROJECT_ID:
        pDiagString = "INVALID PROJECT-ID";
        break;

    case CSIST_INVALID_QUALIFIER:
        pDiagString = "INVALID QUALIFIER";
        break;

    case CSIST_INVALID_RUN_ID:
        pDiagString = "INVALID RUN-ID";
        break;

    case CSIST_INVALID_SUBFIELD:
        pDiagString = "INVALID SUBFIELD";
        break;

    case CSIST_INVALID_USER_ID:
        pDiagString = "INVALID USER-ID";
        break;

    case CSIST_KEYS_NOT_ALLOWED:
        pDiagString = "READ/WRITE KEYS NOT ALLOWED";
        break;

    case CSIST_LABEL_NOT_ALLOWED:
        pDiagString = "LABEL NOT ALLOWED";
        break;

    case CSIST_MAX_FIELDS_SUBFIELDS:
        pDiagString = "MAX NUMBER OF FIELDS OR SUBFIELDS EXCEEDED";
        break;

    case CSIST_MAX_SUBFIELD_CHARS:
        pDiagString = "MAX NUMBER OF CHARACTERS EXCEEDED";
        break;

    case CSIST_MISSING_FILE_SPECIFICATION:
        pDiagString = "MISSING FILE SPECIFICATION";
        break;

    case CSIST_NOT_CONTROL_STATEMENT:
    case CSIST_NOT_FOUND:
        assert( false );

    case CSIST_EXTRANEOUS_TEXT:
    case CSIST_SYNTAX_ERROR:
        pDiagString = "SYNTAX ERROR";
        break;

    default:
        //  There are some codes which we trap here, for which we should never be called.
        //  e.g., CSIST_CONTINUED
        assert( false );
    }

    std::stringstream diagStream;
    diagStream << std::setw( 2 ) << std::setfill( '0' ) << m_Index << " " << pDiagString;
    pRunInfo->postToPrint( diagStream.str() );
}



//  Static publics

//  getSetcLogicalOpString()
const char*
CSInterpreter::getSetcLogicalOpString
(
    const SetcLogicalOp     logicalOp
)
{
    switch ( logicalOp )
    {
    case SLOP_NONE:         return "{none}";
    case SLOP_AND:          return "AND";
    case SLOP_OR:           return "OR";
    case SLOP_XOR:          return "XOR";
    }

    return "???";
}


//  getSetcPartialWordString()
const char*
CSInterpreter::getSetcPartialWordString
(
    const SetcPartialWord   partialWord
)
{
    switch ( partialWord )
    {
    case SPAW_NONE:         return "{none}";
    case SPAW_T1:           return "T1";
    case SPAW_T2:           return "T2";
    case SPAW_S2:           return "S2";
    case SPAW_S3:           return "S3";
    case SPAW_S4:           return "S4";
    }

    return "???";
}


//  getStatusDisplayString()
//
//  Converts Status enum to a displayable string
std::string
CSInterpreter::getStatusString
(
    const Status            status
)
{
    switch ( status )
    {
    case CSIST_SUCCESSFUL:                  return "Successful";
    case CSIST_CONTINUED:                   return "Incomplete Image";
    case CSIST_EXTRANEOUS_TEXT:             return "Extraneous Text";
    case CSIST_FACILITIES_RESULT:           return "See Facilities Result";
    case CSIST_ILLEGAL_CONTINUATION:        return "Illegal Continuation";
    case CSIST_ILLEGAL_OPTION:              return "Illegal Option";
    case CSIST_INTERRUPTED:                 return "Interrupted";
    case CSIST_INVALID_ACCOUNT_ID:          return "Invalid Account Id";
    case CSIST_INVALID_COMMAND_CODE:        return "Invalid Command Code";
    case CSIST_INVALID_CYCLE:               return "Invalid Cycle";
    case CSIST_INVALID_DIRECTORY_ID:        return "Invalid Directory Id";
    case CSIST_INVALID_FILE_NAME:           return "Invalid File Name";
    case CSIST_INVALID_INTERNAL_NAME:       return "Invalid Internal Name";
    case CSIST_INVALID_KEY:                 return "Invalid Key";
    case CSIST_INVALID_LABEL:               return "Invalid Label";
    case CSIST_INVALID_OPTION:              return "Invalid Option";
    case CSIST_INVALID_OPTION_COMBINATION:  return "Invalid Option Combination";
    case CSIST_INVALID_PROJECT_ID:          return "Invalid Project Id";
    case CSIST_INVALID_QUALIFIER:           return "Invalid Qualifier";
    case CSIST_INVALID_RUN_ID:              return "Invalid Run Id";
    case CSIST_INVALID_SUBFIELD:            return "Invalid Subfield";
    case CSIST_INVALID_USER_ID:             return "Invalid User Id";
    case CSIST_KEYS_NOT_ALLOWED:            return "Keys Not Allowed";
    case CSIST_LABEL_NOT_ALLOWED:           return "Label Not Allowed";
    case CSIST_MAX_FIELDS_SUBFIELDS:        return "Max Fields or Subfields";
    case CSIST_MAX_SUBFIELD_CHARS:          return "Max Subfield Chars";
    case CSIST_MISSING_FILE_SPECIFICATION:  return "Missing File Specification";
    case CSIST_NOT_ALLOWED:                 return "Not Allowed";
    case CSIST_NOT_CONTROL_STATEMENT:       return "Not A Control Statement";
    case CSIST_NOT_FOUND:                   return "Not Found";
    case CSIST_SYNTAX_ERROR:                return "Syntax Error";
    }

    return "???";
}


//  extraction operator
//
//  for debugging use only
#ifdef _DEBUG
std::ostream&
operator<<
(
    std::ostream&           stream,
    const CSInterpreter&    csInterpreter
)
{
    const FacilitiesManager::FieldList& fields = csInterpreter.getAdditionalFields();

    switch ( csInterpreter.getCommand() )
    {
    case CSInterpreter::CMD_INIT:
        stream << "CMD_INIT - initial mode, no command scanned yet" << std::endl;
        break;

    case CSInterpreter::CMD_EMPTY:
        stream << "CMD_EMPTY" << std::endl;
        break;

    case CSInterpreter::CMD_ASG:
        stream << "Command:ASG: label=" << csInterpreter.getLabel()
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << " addlFields:";
        for ( INDEX fx = 0; fx < fields.size(); ++fx )
        {
            stream << ",";
            for ( INDEX sx = 0; sx < fields[fx].size(); ++sx )
            {
                if ( sx > 0 )
                    stream << "/";
                stream << fields[fx][sx];
            }
        }
        stream << std::endl;
        break;

    case CSInterpreter::CMD_CAT:
        stream << "Command:CAT: label=" << csInterpreter.getLabel()
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << " addlFields:";
        for ( INDEX fx = 0; fx < fields.size(); ++fx )
        {
            stream << ",";
            for ( INDEX sx = 0; sx < fields[fx].size(); ++sx )
            {
                if ( sx > 0 )
                    stream << "/";
                stream << fields[fx][sx];
            }
        }
        stream << std::endl;
        break;

    case CSInterpreter::CMD_DIR:
        stream << "CMD_DIR: label=" << csInterpreter.getLabel() << std::endl;
        break;

    case CSInterpreter::CMD_FIN:
        stream << "CMD_FIN: label=" << csInterpreter.getLabel() << std::endl;
        break;

    case CSInterpreter::CMD_LABEL:
        stream << "CMD_LABEL: label=" << csInterpreter.getLabel() << std::endl;
        break;

    case CSInterpreter::CMD_LOG:
        stream << "CMD_LOG: label=" << csInterpreter.getLabel() << std::endl;
        stream << "  Text=" << csInterpreter.getText() << std::endl;
        break;

    case CSInterpreter::CMD_MSG:
        stream << "CMD_MSG: label=" << csInterpreter.getLabel()
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << "  Text=" << csInterpreter.getText() << std::endl;
        break;

    case CSInterpreter::CMD_QUAL:
        stream << "CMD_QUAL: label=" << csInterpreter.getLabel()
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << "  Qualifier=" << csInterpreter.getQualifier() << std::endl;
        stream << std::endl;
        break;

    case CSInterpreter::CMD_RINFO:
        stream << "CMD_RINFO: label=" << csInterpreter.getLabel() << std::endl;
        break;

    case CSInterpreter::CMD_RUN:
        stream << "CMD_RUN:"
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << "  Run-id     = " << csInterpreter.getRunId() << std::endl;
        stream << "  Account-id = " << csInterpreter.getAccountId() << std::endl;
        stream << "  Project-id = " << csInterpreter.getProjectId() << std::endl;
        stream << "  User-id    = " << csInterpreter.getUserId() << std::endl;
        stream << "  Sched-Pri  = " << csInterpreter.getSchedulingPriority() << std::endl;
        stream << "  Proc-Pri   = " << csInterpreter.getProcessorDispatchingPriority() << std::endl;
        break;

    case CSInterpreter::CMD_SETC:
        stream << "CMD_SETC: label=" << csInterpreter.getLabel() << std::endl;
        stream << "  LogicalOp   = " << csInterpreter.getLogicalOp() << std::endl;
        stream << "  Value       = " << std::oct << csInterpreter.getValue() << std::endl;
        stream << "  PartialWord = " << csInterpreter.getPartialWord() << std::endl;
        break;

    case CSInterpreter::CMD_USE:
        stream << "CMD_USE: label=" << csInterpreter.getLabel()
            << " opts=" << execGetOptionsString( csInterpreter.getOptions() ) << std::endl;
        stream << "  FileSpec1=" << csInterpreter.getFileSpec1().toString() << std::endl;
        stream << "  FileSpec2=" << csInterpreter.getFileSpec2().toString() << std::endl;
        break;
    }

    return stream;
}
#endif

