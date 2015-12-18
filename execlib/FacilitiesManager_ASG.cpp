//  Facilities class implementation
//  Subfunctions for @ASG



#include    "execlib.h"



//  private methods

//  asgAllocateInitialReserve()
//
//  Attempts to allocate the initial reserve for the given facility item
//
//  Parameters:
//      context:            Context in which this command operates
//      pDiskItem:          pointer to DiskFacilityItem to operate upon
inline bool
FacilitiesManager::asgAllocateInitialReserve
(
    const Context&              context,
    DiskFacilityItem* const     pDiskItem
) const
{
    if ( !pDiskItem->getWriteInhibitedFlag() && ( pDiskItem->getInitialGranules() > 0 ) )
    {
        COUNT tracksPerGranule = pDiskItem->getGranularity() == MSGRAN_POSITION ? 64 : 1;
        TRACK_COUNT initialTracks = pDiskItem->getInitialGranules() * tracksPerGranule;
        MFDManager::Result mfdResult = m_pMFDManager->allocateFileTracks( context.m_pActivity, pDiskItem, 0, initialTracks );
        if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
        {
            logMFDError( context, "FacilitiesManager::asgAllocateInitialReserve", pDiskItem, mfdResult );
            return false;
        }
    }

    return true;
}


//  asgAlreadyAssignedDiskExec()
//
//  Calling code has determined that the requested mass-storage file is already assigned.
//  Set appropriate status code and bit mask contents, then verify other options / eqp types / etc,
//  and update dyanamic settings attached to this fact item appropriately (or error out).
//
//  For @ASG,A @ASG,C and @ASG,P (not @ASG,T)
//  Verifications are assumed to have passed - this is the execution of the request
//
//  Parameters:
//      context:                Context in which this command operates
//      fileSpecification:      effective file specification
//      itFacItem:              iterator indicating the FACITEM representing the already-assigned file
//      fileSetInfo:            from MFDManager
//      fileSetCycleEntryIndex: index of appropriate cycle entry in fileSetInfo
//      fileCycleInfo:          from MFDManager
//      changes:                ref to AlreadyAssignedChangesDisk describing any changes to be made to the
//                                  disk file attributes
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgAlreadyAssignedDiskExec
(
    const Context&                              context,
    const FileSpecification&                    fileSpecification,
    const CITFACITEMS                           itFacItem,
    const MFDManager::FileSetInfo&              fileSetInfo,
    const INDEX                                 fileSetCycleEntryIndex,
    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo,
    const AlreadyAssignedChangesDisk&           changes
) const
{
    //  Update any changed fields in the MFD (incl. init reserve, max grans, but also others)
    if ( changes.m_SetExclusiveUse || changes.m_UpdateInitialReserve || changes.m_UpdateMaxGranules )
    {
        const bool mfdExclusive = changes.m_SetExclusiveUse ? true : fileCycleInfo.m_ExclusiveUse;
        const COUNT32 mfdInitialReserve = changes.m_UpdateInitialReserve ? changes.m_NewInitialReserve : fileCycleInfo.m_InitialReserve;
        const COUNT32 mfdMaxGranules = changes.m_UpdateMaxGranules ? changes.m_NewMaxGranules : fileCycleInfo.m_MaxGranules;
        MFDManager::Result mfdResult = m_pMFDManager->updateFileCycle( context.m_pActivity,
                                                                       fileCycleInfo.m_DirectorySectorAddresses[0],
                                                                       mfdExclusive,
                                                                       mfdInitialReserve,
                                                                       mfdMaxGranules );
        if ( !checkMFDResult( mfdResult, context.m_pResult ) )
        {
            logMFDError( context, "FacilitiesManager::asgAlreadyAssignedDiskExec()", itFacItem->second, mfdResult );
            return false;
        }
    }

    DiskFacilityItem* pDiskItem = dynamic_cast<DiskFacilityItem*>( itFacItem->second );

    //  X-option newly-specified?
    if ( changes.m_SetExclusiveUse )
        pDiskItem->setExclusiveFlag( true );

    //  Set flag to automatically release the run next time a task terminates? (I option)
    if ( changes.m_SetReleaseOnTaskEnd )
        pDiskItem->setReleaseFlag( true );

    //  (maybe) update intial reserve and max
    if ( changes.m_UpdateInitialReserve )
        pDiskItem->setInitialGranules( changes.m_NewInitialReserve );
    if ( changes.m_UpdateMaxGranules )
        pDiskItem->setMaximumGranules( changes.m_NewMaxGranules );

    //  File is (obviously) already assigned
    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

    //  File already assigned to another run warning?
    //  We know this by seeing the assign count > 0 in the FileCycleInfo object.
    //  (in the MFD it is actually one greater than what is in fileCycleInfo,
    //  so we are actually checking whether the *new* current assign count is > 1).
    if ( fileCycleInfo.m_CurrentAssignCount > 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ASSIGNED_TO_ANOTHER_RUN );
        context.m_pResult->m_StatusBitMask.logicalOr( 0000000100000ll );
    }

    return asgPostProcessing( context, fileSpecification, pDiskItem );
}


//  asgCatalog()
//
//  Used as the root driver for all code which handles @ASG,C and @ASG,U.
//  Note that, per the fact that @CAT of relative cycle < +1 makes no sense, we disallow it here.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgCatalog
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields
) const
{
    //  Global option checking, so we don't go any further if something is amiss
    UINT32 allowedOptions = OPTB_B | OPTB_C | OPTB_G | OPTB_I | OPTB_N | OPTB_P | OPTB_R | OPTB_S | OPTB_U | OPTB_V | OPTB_W | OPTB_Z;
    if ( !checkOptions( options, allowedOptions, context.m_pResult ) )
        return false;

    //  Is the file already assigned, either by absolute or relative file cycle?
    //  If so, we're limited to maybe changing a few parameters for disk (and not really anything for tape).
    CITFACITEMS itFacItem;
    if ( isFileAssigned( fileSpecification, *context.m_pRunInfo, &itFacItem ) )
        return asgCatalogAlreadyAssigned( context, options, fileSpecification, additionalFields, itFacItem );

    //  If there's an equipment type given, validate it.
    const EquipmentType* pEquipmentType = checkEquipmentType( context, additionalFields );
    if ( !pEquipmentType && context.m_pResult->containsErrorStatusCode() )
        return false;

    //  See whether a file set exists
    MFDManager::FileSetInfo fileSetInfo;
    MFDManager::Result mfdResult = m_pMFDManager->getFileSetInfo( fileSpecification.m_Qualifier,
                                                                  fileSpecification.m_FileName,
                                                                  &fileSetInfo );
    if ( ( mfdResult.m_Status != MFDManager::MFDST_NOT_FOUND )
        && ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL ) )
    {
        checkMFDResult( mfdResult, context.m_pResult );
        logMFDError( context, "FacilitiesManager::asgCatalog()", fileSpecification, mfdResult );
        return false;
    }

    bool fileSetExists = ( mfdResult.m_Status == MFDManager::MFDST_SUCCESSFUL );

    //  Call an appropriate subfunction
    if ( !pEquipmentType )
        return asgCatalogNewUnknown( context, options, additionalFields, fileSpecification, fileSetExists ? &fileSetInfo : 0 );
    else if ( pEquipmentType->m_EquipmentCategory == ECAT_DISK )
        return asgCatalogNewDisk( context, options, additionalFields, fileSpecification, pEquipmentType, fileSetExists ? &fileSetInfo : 0 );
    else if ( pEquipmentType->m_EquipmentCategory == ECAT_TAPE )
        return asgCatalogNewTape( context, options, additionalFields, fileSpecification, pEquipmentType, fileSetExists ? &fileSetInfo : 0 );

    //  We should never be able to get here - all assign mnemonics are disk or tape.
    std::string logMsg = "FacilitiesManager_ASG:asgCatalog():Problem with equipment type:";
    logMsg += pEquipmentType->m_pMnemonic;
    SystemLog::write( logMsg );
    m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
    return false;
}


//  asgCatalogAlreadyAssigned()
//
//  Processing an @ASG,C or @ASG,P on a file which is already assigned to the run.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgCatalogAlreadyAssigned
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS&          itFacItem
) const
{
    //  Redirect to the disk- or tape-specific function
    FacilityItem* pFacItem = itFacItem->second;
    if ( pFacItem->isSectorMassStorage() || pFacItem->isWordMassStorage() )
        return asgCatalogAlreadyAssignedDisk( context, options, fileSpecification, additionalFields, itFacItem );
    else if ( pFacItem->isTape() )
        return asgCatalogAlreadyAssignedTape( context, options, fileSpecification, additionalFields, itFacItem );

    //  Not tape, not disk - this must be a temporary assignment of a non-standard or unit record device
    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );
    return true;
}


//  asgCatalogAlreadyAssignedDisk()
//
//  Processing an @ASG,C or @ASG,P on a file which is already assigned to the run.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgCatalogAlreadyAssignedDisk
(
    const Context&                      context,
    const UINT32                        options,
    const FileSpecification&            fileSpecification,
    const FieldList&                    additionalFields,
    const CITFACITEMS&                  itFacItem
) const
{
    //  Retrieve file set and file cycle info.
    MFDManager::MassStorageFileCycleInfo fileCycleInfo;
    MFDManager::FileSetInfo fileSetInfo;
    INDEX cycleEntryIndex;

    if ( !findLeadAndMainItemInfoDisk( fileSpecification, &fileSetInfo, &cycleEntryIndex, &fileCycleInfo ) )
    {
        SystemLog::write( "FacilitiesManager_ASG::asgCatalogAlreadyAssignedDisk():MFD not successful" );
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        m_pExec->stopExec( Exec::SC_DIRECTORY_ERROR );
        return false;
    }

    StandardFacilityItem* pFacItem = dynamic_cast<StandardFacilityItem*>( itFacItem->second );
    AlreadyAssignedChangesDisk changes;

    if ( !asgCatalogAlreadyAssignedDiskCheck( context, options, additionalFields, pFacItem, &changes ) )
        return false;
    return asgAlreadyAssignedDiskExec( context, fileSpecification, itFacItem, fileSetInfo, cycleEntryIndex, fileCycleInfo, changes );
}


//  asgCatalogAlreadyAssignedDiskCheck()
//
//  Processing an @ASG,C or @ASG,P on a file which is already assigned to the run
//  --  these are the validity checks to make sure we can/are able to do this.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pChanges:               Object where we record any changes to be made to the FACITEM and/or MFD item(s)
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgCatalogAlreadyAssignedDiskCheck
(
    const Context&                      context,
    const UINT32                        options,
    const FieldList&                    additionalFields,
    const StandardFacilityItem* const   pFacItem,
    AlreadyAssignedChangesDisk* const   pChanges
) const
{
    //  Verify C / U option matching - current assignment must match existing assignment.
    //  Being here, we can assume the current assignment already has either C or U,
    //  So a simple comparison of masked options is sufficient.
    UINT32 maskedExisting = pFacItem->getAssignOptions() & (OPTB_C | OPTB_U);
    UINT32 maskedNew = options & (OPTB_C | OPTB_U);
    if ( maskedExisting != maskedNew )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
        return false;
    }

    //  Ignore options which would be acceptable, excepting that the file is already assigned.
    UINT32 mask = OPTB_C | OPTB_I | OPTB_X | OPTB_U;
    if ( (options & mask) != options )
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_MISC );

    //  I-option specified?  Deal with it...
    if ( ((options & OPTB_I) != 0) && !pFacItem->getReleaseFlag() )
        pChanges->m_SetReleaseOnTaskEnd = true;

    DiskSubfieldInfo diskInfo;
    if ( !checkDiskSubfields( context, additionalFields, 0, pFacItem, &diskInfo, false ) )
        return false;

    pChanges->m_UpdateInitialReserve = diskInfo.m_InitialReserveSpecified;
    pChanges->m_NewInitialReserve = diskInfo.m_InitialReserve;
    pChanges->m_UpdateMaxGranules = diskInfo.m_MaxGranulesSpecified;
    pChanges->m_NewMaxGranules = diskInfo.m_MaxGranules;

    return true;
}


//  asgCatalogAlreadyAssignedTape()
//
//  Processing an @ASG,C or @ASG,P on a file which is already assigned to the run.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgCatalogAlreadyAssignedTape
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS&          itFacItem
) const
{
    //  Per PRM, we don't change any settings for already-assigned tape files.
    //  If Neither C or U given, emit 241433 FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE
    if ( (options & (OPTB_C | OPTB_U)) == 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
        return false;
    }

    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED_THIS_IMAGE_IGNORED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

    //  Do post-processing (I know, it's a NOP, but do it anyway)
    return asgPostProcessing( context, fileSpecification, itFacItem->second );
}


//  asgCatalogNewDisk()
//
//  Assign for catalog of a disk file.
//  Either the caller specified disk equipment, or it was implied.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewDisk
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Option checking specific to disk
    UINT32 allowedOptions = OPTB_B | OPTB_C | OPTB_G | OPTB_I | OPTB_P | OPTB_R | OPTB_S | OPTB_U | OPTB_V | OPTB_W | OPTB_Z;
    if ( !checkOptions( options, allowedOptions, context.m_pResult ) )
        return false;

    if ( pFileSetInfo )
        return asgCatalogNewDiskCycle( context, options, additionalFields, fileSpecification, pEquipmentType, pFileSetInfo );
    else
        return asgCatalogNewDiskSetCycle( context, options, additionalFields, fileSpecification, pEquipmentType );
}


//  asgCatalogNewDiskCycle()
//
//  Assign for catalog of a disk file cycle for an existing file set.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewDiskCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Make sure read and write keys were specified, if they exist on the fileset
    bool readDisabled = false;
    bool writeDisabled = false;
    if ( !checkKeys( context, fileSpecification, *pFileSetInfo, true, &readDisabled, &writeDisabled ) )
        return false;

    //  If checkKeys produced either disable, we do not proceed.
    if ( readDisabled || writeDisabled )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0400400000000ll );
        return false;
    }

    //  Check constraints on requested f-cycle
    UINT16 absoluteCycle = 0;
    if ( !checkCycleConstraints( context, fileSpecification, *pFileSetInfo, &absoluteCycle ) )
        return false;

    //  Validate additional subfields
    DiskSubfieldInfo dsInfo;
    dsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkDiskSubfields( context, additionalFields, 0, 0, &dsInfo, false ) )
        return false;

    //  Create file cycle
    bool saveOnCheckpoint = (options & OPTB_B) ? true : false;
    bool privateFile = !((options & OPTB_P) ? true : false);
    bool writeInhibited = (options & OPTB_R) ? true : false;
    bool storeThrough = (options & OPTB_S) ? true : false;
    bool positionGranularity = dsInfo.m_GranularitySpecified ? ( dsInfo.m_Granularity == MSGRAN_POSITION ) : MSGRAN_TRACK;
    bool wordAddressable = ( pEquipmentType->m_EquipmentTypeGroup == FACETG_WORD_DISK );
    bool readInhibited = (options & OPTB_W) ? true : false;
    bool unloadInhibited = (options & OPTB_V) ? true : false;
    COUNT32 initialReserve = dsInfo.m_InitialReserveSpecified ? dsInfo.m_InitialReserve : 0;
    COUNT32 maxGranules = dsInfo.m_MaxGranulesSpecified ? dsInfo.m_MaxGranules : m_DefaultMaxGranules;
    bool releaseFlag = (options & OPTB_I) == 0 ? false : true;      //  for later

    MFDManager::Result mfdResult = m_pMFDManager->createFileCycle( context.m_pActivity,
                                                                   pFileSetInfo->m_DSAddresses[0],
                                                                   context.m_pRunInfo->getAccountId(),
                                                                   saveOnCheckpoint,
                                                                   storeThrough,
                                                                   positionGranularity,
                                                                   wordAddressable,
                                                                   pEquipmentType->m_pMnemonic,
                                                                   unloadInhibited,
                                                                   privateFile,
                                                                   readInhibited,
                                                                   writeInhibited,
                                                                   absoluteCycle,
                                                                   initialReserve,
                                                                   maxGranules,
                                                                   true );

    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskCycle(a)", fileSpecification, mfdResult );
        return false;
    }

    //  Need to get file cycle info here
    MFDManager::MassStorageFileCycleInfo fileCycleInfo;
    MFDManager::FileSetInfo fileSetInfo;
    INDEX cycleEntryIndex;

    if ( !findLeadAndMainItemInfoDisk( fileSpecification, &fileSetInfo, &cycleEntryIndex, &fileCycleInfo ) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    //  Update MFD
    FileAllocationTable* pFileAllocationTable = 0;
    mfdResult = m_pMFDManager->assignFileCycle( context.m_pActivity,
                                                fileCycleInfo.m_DirectorySectorAddresses[0],
                                                true,       //  always exclusive
                                                initialReserve,
                                                maxGranules,
                                                &pFileAllocationTable );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskCycle(b)", fileSpecification, mfdResult );
        return false;
    }

    //  Create FACITEM and update RunInfo
    DiskFacilityItem* pFacItem = 0;
    INT8 relativeFileCycle = fileSpecification.m_RelativeCycleSpecified ? fileSpecification.m_Cycle : 0;
    EquipmentCode equipCode = fileCycleInfo.m_WordAddressable ? ECODE_WORD_DISK : ECODE_SECTOR_DISK;

    pFacItem = new DiskFacilityItem( fileSpecification.m_FileName,
                                     fileSpecification.m_Qualifier,
                                     equipCode,
                                     options,
                                     releaseFlag,
                                     fileCycleInfo.m_AbsoluteCycle,
                                     fileSpecification.m_AbsoluteCycleSpecified,
                                     true,      //  always exclusive
                                     false,     //  not an existing file
                                     fileCycleInfo.m_DirectorySectorAddresses[0],
                                     readInhibited,
                                     false,     //  read key not needed
                                     relativeFileCycle,
                                     fileSpecification.m_RelativeCycleSpecified,
                                     false,     //  not a temporary file
                                     writeInhibited,
                                     false,
                                     pFileAllocationTable,
                                     positionGranularity ? MSGRAN_POSITION : MSGRAN_TRACK,
                                     0,
                                     0,
                                     initialReserve,
                                     maxGranules );

    context.m_pRunInfo->insertFacilityItem( pFacItem );
    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgCatalogNewDiskSetCycle()
//
//  Assign for catalog of a disk file cycle for which there is no existing file set.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewDiskSetCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType
) const
{
    //  Check to ensure f-cycle is valid.
    //  We accept any valid absolute cycle, but only +1 for a relative cycle.
    //  If not specified, we default to absolute cycle 1.
    if ( fileSpecification.m_AbsoluteCycleSpecified || fileSpecification.m_RelativeCycleSpecified )
    {
        if ( !checkCycleConstraints( context, fileSpecification ) )
            return false;
    }

    UINT16 absoluteCycle = fileSpecification.m_AbsoluteCycleSpecified ? fileSpecification.m_Cycle : 1;

    //  Check additional subfields
    DiskSubfieldInfo dsInfo;
    dsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkDiskSubfields( context, additionalFields, 0, 0, &dsInfo, false ) )
        return false;

    //  Create file set
    DSADDR leadItem0Addr = 0;
    MFDManager::Result mfdResult = m_pMFDManager->createFileSet( context.m_pActivity,
                                                                 fileSpecification.m_Qualifier,
                                                                 fileSpecification.m_FileName,
                                                                 context.m_pRunInfo->getProjectId(),
                                                                 fileSpecification.m_ReadKey,
                                                                 fileSpecification.m_WriteKey,
                                                                 MFDManager::FILETYPE_MASS_STORAGE,
                                                                 (options & OPTB_G) == OPTB_G,
                                                                 &leadItem0Addr );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() createFileSet failed", fileSpecification, mfdResult );
        return false;
    }

    //  Retrieve file set info to get the file set's DSAddresses (we need the lead item sector 0 addr)
    MFDManager::FileSetInfo fileSetInfo;
    mfdResult = m_pMFDManager->getFileSetInfo( fileSpecification.m_Qualifier, fileSpecification.m_FileName, &fileSetInfo );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() getFileSetInfo failed(1)", fileSpecification, mfdResult );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    //  Create file cycle
    bool saveOnCheckpoint = (options & OPTB_B) ? true : false;
    bool privateFile = !((options & OPTB_P) ? true : false);
    bool writeInhibited = (options & OPTB_R) ? true : false;
    bool storeThrough = (options & OPTB_S) ? true : false;
    bool positionGranularity = dsInfo.m_GranularitySpecified ? ( dsInfo.m_Granularity == MSGRAN_POSITION ) : MSGRAN_TRACK;
    bool wordAddressable = ( pEquipmentType->m_EquipmentTypeGroup == FACETG_WORD_DISK );
    bool readInhibited = (options & OPTB_W) ? true : false;
    bool unloadInhibited = (options & OPTB_V) ? true : false;
    COUNT32 initialReserve = dsInfo.m_InitialReserveSpecified ? dsInfo.m_InitialReserve : 0;
    COUNT32 maxGranules = dsInfo.m_MaxGranulesSpecified ? dsInfo.m_MaxGranules : m_DefaultMaxGranules;
    bool releaseFlag = (options & OPTB_I) == 0 ? false : true;      //  for later

    mfdResult = m_pMFDManager->createFileCycle( context.m_pActivity,
                                                fileSetInfo.m_DSAddresses[0],
                                                context.m_pRunInfo->getAccountId(),
                                                saveOnCheckpoint,
                                                storeThrough,
                                                positionGranularity,
                                                wordAddressable,
                                                pEquipmentType->m_pMnemonic,
                                                unloadInhibited,
                                                privateFile,
                                                readInhibited,
                                                writeInhibited,
                                                absoluteCycle,
                                                initialReserve,
                                                maxGranules,
                                                true );

    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() createFileCycle failed", fileSpecification, mfdResult );
        return false;
    }

    //  Need to reload file set info to pick up new file cycle, and get file cycle info
    mfdResult = m_pMFDManager->getFileSetInfo( fileSpecification.m_Qualifier, fileSpecification.m_FileName, &fileSetInfo );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() getFileSetInfo failed(2)", fileSpecification, mfdResult );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    MFDManager::MassStorageFileCycleInfo fileCycleInfo;
    INDEX cycleEntryIndex;
    if ( !findExistingCycleEntryIndex( fileSpecification, fileSetInfo, &cycleEntryIndex ) )
    {
        SystemLog::write( "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() findExistingCycleEntryIndex failed" );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    mfdResult = m_pMFDManager->getMassStorageFileCycleInfo( fileSetInfo.m_CycleEntries[cycleEntryIndex].m_MainItem0Addr, &fileCycleInfo );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        std::string logMsg = "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle() getMassStorageFileCycleInfo failed:";
        logMsg += MFDManager::getResultString( mfdResult );
        SystemLog::write( logMsg );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    //  Update MFD
    FileAllocationTable* pFileAllocationTable = 0;
    mfdResult = m_pMFDManager->assignFileCycle( context.m_pActivity,
                                                fileCycleInfo.m_DirectorySectorAddresses[0],
                                                true,       //  always exclusive
                                                initialReserve,
                                                maxGranules,
                                                &pFileAllocationTable );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgCatalogNewDiskSetCycle assignFileCycle failed", fileSpecification, mfdResult );
        return false;
    }

    //  Create FACITEM and update RunInfo
    DiskFacilityItem* pFacItem = 0;
    INT8 relativeFileCycle = fileSpecification.m_RelativeCycleSpecified ? fileSpecification.m_Cycle : 0;
    EquipmentCode equipCode = fileCycleInfo.m_WordAddressable ? ECODE_WORD_DISK : ECODE_SECTOR_DISK;

    pFacItem = new DiskFacilityItem( fileSpecification.m_FileName,
                                     fileSpecification.m_Qualifier,
                                     equipCode,
                                     options,
                                     releaseFlag,
                                     fileCycleInfo.m_AbsoluteCycle,
                                     fileSpecification.m_AbsoluteCycleSpecified,
                                     true,      //  always exclusive
                                     false,     //  not yet existing
                                     fileCycleInfo.m_DirectorySectorAddresses[0],
                                     readInhibited,
                                     false,     //  read key not needed
                                     relativeFileCycle,
                                     fileSpecification.m_RelativeCycleSpecified,
                                     false,     //  not temporary
                                     writeInhibited,
                                     false,     //  write key not needed
                                     pFileAllocationTable,
                                     positionGranularity ? MSGRAN_POSITION : MSGRAN_TRACK,
                                     0,
                                     0,
                                     initialReserve,
                                     maxGranules );

    context.m_pRunInfo->insertFacilityItem( pFacItem );
    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgCatalogNewTape()
//
//  Assign for catalog of a tape file.
//  Either the caller specified tape equipment, or it was implied.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewTape
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Options specific to tape
    UINT32 allowedOptions = OPTB_B | OPTB_C | OPTB_G | OPTB_I | OPTB_N | OPTB_P | OPTB_R | OPTB_U | OPTB_W | OPTB_Z;
    if ( !checkOptions( options, allowedOptions, context.m_pResult ) )
        return false;

    if ( pFileSetInfo )
        return asgCatalogNewTapeCycle( context, options, additionalFields, fileSpecification, pEquipmentType, pFileSetInfo );
    else
        return asgCatalogNewTapeSetCycle( context, options, additionalFields, fileSpecification, pEquipmentType );
}


//  asgCatalogNewTapeCycle()
//
//  Assign for catalog of a tape file cycle for an existing file set.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewTapeCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    return true;//TODO:TAPE
}


//  asgCatalogNewTapeSetCycle()
//
//  Assign for catalog of a tape file cycle for which there is no existing file set.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pEquipmentType:         pointer to EquipmentType object for this assign
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewTapeSetCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const EquipmentType*            pEquipmentType
) const
{
    return true;//TODO:TAPE
}


//  asgCatalogNewUnknown()
//
//  Assigns for cataloging a file if possible.  The caller did not specify an equipment type.
//  Thus, if a file set is already cataloged for the qual*file combination, we use that fileset's general
//  type (TAPE or WORD-MASS-STORAGE) to select the system-configured default equipment type.
//  Otherwise, we assume (sector-addressable) mass storage.
//  We already know that the file has not already been assigned.
//  It may have absolute file cycle, or relative file cycle (we allow only relative cycle +1).
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      fileSpecification:      effective file specification
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists, else 0
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::asgCatalogNewUnknown
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    const FileSpecification&        fileSpecification,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  If a fileset exists and it's tape, use the default tape equipment type to catalog a new tape cycle.
    if ( pFileSetInfo && pFileSetInfo->m_FileType == MFDManager::FILETYPE_TAPE )
        return asgCatalogNewTape( context, options, additionalFields, fileSpecification, m_pDefaultEquipTape, pFileSetInfo );

    //  Otherwise, fileset or not, we're going to use the default sector-addressable equipment type
    //  to catalog a new disk cycle.
    return asgCatalogNewDisk( context, options, additionalFields, fileSpecification, m_pDefaultEquipMassStorage, pFileSetInfo );
}


//  asgExisting()
//
//  Used for validating the various fields and options for assigning an existing file cycle.
//  We assume that the A option was given or impied.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false... if anyone cares.
bool
FacilitiesManager::asgExisting
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields
) const
{
    if ( fileSpecification.m_AbsoluteCycleSpecified )
        return asgExistingAbsolute( context, options, fileSpecification, additionalFields );
    else
        return asgExistingRelative( context, options, fileSpecification, additionalFields );
}


//  asgExistingAbsolute()
//
//  Attempts to assign the indicated file, using the indicated absolute file cycle.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pResult:            pointer to Result object which we update
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingAbsolute
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields
) const
{
    //  Search RunInfo to see if the requested file is already assigned to the run.
    CITFACITEMS itFacItem;
    if ( isFileAssigned( fileSpecification, *context.m_pRunInfo, &itFacItem ) )
        return asgExistingAlreadyAssigned( context, options, fileSpecification, additionalFields, itFacItem );

    //  Search MFD to see whether the file set exists - if not, reject the assignment.
    MFDManager::FileSetInfo fileSetInfo;
    INDEX fileSetCycleEntryIndex;

    if ( !findLeadItemInfo( fileSpecification, &fileSetInfo, &fileSetCycleEntryIndex ) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_NOT_CATALOGED );
        context.m_pResult->m_StatusBitMask.setW( 0400010000000 );
        return false;
    }

    //  Assign the file to the user's run
    return asgExistingNew( context, options, fileSpecification, additionalFields, fileSetInfo, fileSetCycleEntryIndex );
}


//  asgExistingAlreadyAssigned()
//
//  Thin wrapper which redirects to the appropriate Disk or Tape version of this function.
//  If neither disk nor tape, we merely post a warning.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      itFacItem:          iterator indicating the FACITEM representing the already-assigned file
//
//  Returns true if successful, else false
inline bool
FacilitiesManager::asgExistingAlreadyAssigned
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS           itFacItem
) const
{
    //  Redirect to the disk- or tape-specific function
    FacilityItem* pFacItem = itFacItem->second;
    if ( pFacItem->isSectorMassStorage() || pFacItem->isWordMassStorage() )
        return asgExistingAlreadyAssignedDisk( context, options, fileSpecification, additionalFields, itFacItem );
    else if ( pFacItem->isTape() )
        return asgExistingAlreadyAssignedTape( context, options, fileSpecification, additionalFields, itFacItem );

    //  Not tape, not disk - this must be a temporary assignment of a non-standard or unit record device
    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );
    return true;
}


//  asgExistingAlreadyAssignedDisk()
//
//  Calling code has determined that the requested mass-storage file is already assigned.
//  Set appropriate status code and bit mask contents, then verify other options / eqp types / etc,
//  and update dyanamic settings attached to this fact item appropriately (or error out).
//
//  Split into two sections - checks and execution.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      itFacItem:          iterator indicating the FACITEM representing the already-assigned file
//
//  Returns true if successful, else false
inline bool
FacilitiesManager::asgExistingAlreadyAssignedDisk
(
    const Context&                  context,
    const UINT32                    options,
    const FileSpecification&        fileSpecification,
    const FieldList&                additionalFields,
    const CITFACITEMS               itFacItem
) const
{
    //  Retrieve file set and file cycle info.
    MFDManager::MassStorageFileCycleInfo fileCycleInfo;
    MFDManager::FileSetInfo fileSetInfo;
    INDEX cycleEntryIndex;

    if ( !findLeadAndMainItemInfoDisk( fileSpecification, &fileSetInfo, &cycleEntryIndex, &fileCycleInfo ) )
    {
        SystemLog::write( "FacilitiesManager_ASG::asgExistingAlreadyAssignedDisk():findLeadAndMainItemInfoDisk failed" );
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        m_pExec->stopExec(Exec::SC_DIRECTORY_ERROR);
        return false;
    }

    AlreadyAssignedChangesDisk changes;
    bool result = asgExistingAlreadyAssignedDiskCheck( context,
                                                       options,
                                                       fileSpecification,
                                                       additionalFields,
                                                       itFacItem,
                                                       cycleEntryIndex,
                                                       fileCycleInfo,
                                                       &changes );
    if ( result )
        result = asgAlreadyAssignedDiskExec( context,
                                             fileSpecification,
                                             itFacItem,
                                             fileSetInfo,
                                             cycleEntryIndex,
                                             fileCycleInfo,
                                             changes );

    return result;
}


//  asgExistingAlreadyAssignedDiskCheck()
//
//  Calling code has determined that the requested mass-storage file is already assigned.
//  Set appropriate status code and bit mask contents, then verify other options / eqp types / etc,
//  and update dyanamic settings attached to this fact item appropriately (or error out).
//
//  Verification only - does not effect any changes.
//
//  Parameters:
//      context:                Context in which this command operates
//      options:                options given on the @ASG statement
//      fileSpecification:      the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:       additional fields beyond the file specification - the semantics of these fields and
//                                  subfields are dependent upon (to some extent) the options, and more directly,
//                                  to the equipment type of the file involved
//      itFacItem:              iterator indicating the FACITEM representing the already-assigned file
//      fileSetInfo:            from MFDManager
//      fileSetCycleEntryIndex: index of appropriate cycle entry in fileSetInfo
//      fileCycleInfo:          from MFDManager
//      pChanges:               Object where we record any changes to be made to the FACITEM and/or MFD item(s)
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingAlreadyAssignedDiskCheck
(
    const Context&                              context,
    const UINT32                                options,
    const FileSpecification&                    fileSpecification,
    const FieldList&                            additionalFields,
    const CITFACITEMS                           itFacItem,
    const INDEX                                 fileSetCycleEntryIndex,
    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo,
    AlreadyAssignedChangesDisk*                 pChanges
) const
{
    StandardFacilityItem* pFacItem = dynamic_cast<StandardFacilityItem*>( itFacItem->second );

    //  If C or U given, emit 241433 FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE
    if ( options & (OPTB_C | OPTB_U) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
        return false;
    }

    //  Validate options.  This first check allows all acceptable options for normal @ASG,A.
    //  Basically, we are filtering out all non-sense options.
    UINT32 mask = OPTB_A | OPTB_D | OPTB_E | OPTB_K | OPTB_I | OPTB_M | OPTB_Q | OPTB_R | OPTB_X | OPTB_Y | OPTB_Z;
    if ( !checkOptions( options, mask, context.m_pResult ) )
        return false;

    //  Check for conflicting options
    mask = OPTB_E | OPTB_Q | OPTB_Y;
    if ( !checkOptionConflicts( context, options, mask ) )
        return false;

    //  Now ignore options which would be acceptable, excepting that the file is already assigned.
    mask = OPTB_A | OPTB_I | OPTB_X;
    if ( (options & mask) != options )
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_MISC );

    //  I-option specified?  Deal with it...
    if ( ((options & OPTB_I) != 0) && !pFacItem->getReleaseFlag() )
        pChanges->m_SetReleaseOnTaskEnd = true;

    //  Add a warning if user specified X option now and previously as well.
    if ( options & OPTB_X )
    {
        //  Is X option already specified for the assigned file?
        if ( pFacItem->getExclusiveFlag() )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ALREADY_EXCLUSIVELY_ASSIGNED );
            context.m_pResult->m_StatusBitMask.logicalOr( 002000000000LL );
        }
        else
        {
            //  We are trying to apply x-use to a file already assigned *not* with x-use...
            //  Is the file read-only?  If so, the X option is ignored
            if ( !fileCycleInfo.m_WriteInhibited )
            {
                //  Write not inhibited, thus file is not read-only. So...
                //  Is the file assigned to another run besides this one?
                if ( fileCycleInfo.m_CurrentAssignCount > 1 )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_HELD_FOR_NEED_OF_EXCLUSIVE_USE );
                    context.m_pResult->m_StatusBitMask.logicalOr( 000001000000LL );
                    return false;
                }
            }

            pChanges->m_SetExclusiveUse = true;
        }
    }

    DiskSubfieldInfo diskInfo;
    if ( !checkDiskSubfields( context, additionalFields, &fileCycleInfo, pFacItem, &diskInfo, false ) )
        return false;

    pChanges->m_UpdateInitialReserve = diskInfo.m_InitialReserveSpecified;
    pChanges->m_NewInitialReserve = diskInfo.m_InitialReserve;
    pChanges->m_UpdateMaxGranules = diskInfo.m_MaxGranulesSpecified;
    pChanges->m_NewMaxGranules = diskInfo.m_MaxGranules;

    return true;
}


//  asgExistingAlreadyAssignedTape()
//
//  Calling code has determined that the requested tape file is already assigned.
//  We don't really do much here.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      itFacItem:          iterator indicating the FACITEM representing the already-assigned file
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingAlreadyAssignedTape
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS           itFacItem
) const
{
    //  Per PRM, we don't change any settings for already-assigned tape files.
    //  If C or U given, emit 241433 FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE
    if ( options & (OPTB_C | OPTB_U) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
        return false;
    }

    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED_THIS_IMAGE_IGNORED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

    return asgPostProcessing( context, fileSpecification, itFacItem->second );
}


//  asgExistingNew()
//
//  We've done some validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned
//  to his run.  We still have a lot of verification to do, after which we need to
//  update the MFD a little bit, then update the user's RunInfo object as well.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:                options given on the @ASG statement
//      fileSpecification:      the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:       additional fields beyond the file specification - the semantics of these fields and
//                                  subfields are dependent upon (to some extent) the options, and more directly,
//                                  to the equipment type of the file involved
//      fileSetInfo:            reference to a FileSetInfo object created by MFDManager associated with fileCycleInfo below
//      fileCycleInfo:          reference to a FileCycleInfo object created by MFDManager
//      fileCycleEntryIndex:    index into the cycle entry table for the relevant cycle
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingNew
(
    const Context&                  context,
    const UINT32                    options,
    const FileSpecification&        fileSpecification,
    const FieldList&                additionalFields,
    const MFDManager::FileSetInfo&  fileSetInfo,
    const INDEX                     fileCycleEntryIndex
) const
{
    DSADDR mainItem0Addr = fileSetInfo.m_CycleEntries[fileCycleEntryIndex].m_MainItem0Addr;
    if ( fileSetInfo.m_FileType == MFDManager::FILETYPE_MASS_STORAGE )
    {
        MFDManager::MassStorageFileCycleInfo msfcInfo;
        MFDManager::Result mfdResult = m_pMFDManager->getMassStorageFileCycleInfo( mainItem0Addr, &msfcInfo );
        if ( !checkMFDResult( mfdResult, context.m_pResult ) )
        {
            logMFDError( context, "FacilitiesManager_ASG::asgExistingNew(a)", fileSpecification, mfdResult );
            m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
            return false;
        }

        return asgExistingNewDisk( context, options, fileSpecification, additionalFields, fileSetInfo, msfcInfo );
    }
    else
    {
        MFDManager::TapeFileCycleInfo tfcInfo;
        MFDManager::Result mfdResult = m_pMFDManager->getTapeFileCycleInfo( mainItem0Addr, &tfcInfo );
        if ( !checkMFDResult( mfdResult, context.m_pResult ) )
        {
            logMFDError( context, "FacilitiesManager_ASG::asgExistingNew(b)", fileSpecification, mfdResult );
            m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
            return false;
        }

        return asgExistingNewTape( context, options, fileSpecification, additionalFields, fileSetInfo, tfcInfo );
    }
}


//  asgExistingNewDisk()
//
//  We've done some validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned
//  to his run.  We still have a lot of verification to do, after which we need to
//  update the MFD a little bit, then update the user's RunInfo object as well.
//  This is specifically for cataloged mass-storage files.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      fileSetInfo:        reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:      reference to a FileCycleInfo object from MFDManager
//
//  Returns true if successful, else false
inline bool
FacilitiesManager::asgExistingNewDisk
(
    const Context&                              context,
    const UINT32                                options,
    const FileSpecification&                    fileSpecification,
    const FieldList&                            additionalFields,
    const MFDManager::FileSetInfo&              fileSetInfo,
    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo
) const
{
    NewlyAssignedChangesDisk changes;
    bool readKeyNeeded = false;
    bool writeKeyNeeded = false;
    bool readInhibited = false;
    bool writeInhibited = false;

    if ( !asgExistingNewDiskCheck( context,
                                   options,
                                   fileSpecification,
                                   additionalFields,
                                   fileSetInfo,
                                   fileCycleInfo,
                                   &changes,
                                   &readKeyNeeded,
                                   &writeKeyNeeded,
                                   &readInhibited,
                                   &writeInhibited ) )
        return false;

    return asgExistingNewDiskExec( context,
                                   options,
                                   fileSpecification,
                                   additionalFields,
                                   fileSetInfo,
                                   fileCycleInfo,
                                   changes,
                                   readKeyNeeded,
                                   writeKeyNeeded,
                                   readInhibited,
                                   writeInhibited );
}


//  asgExistingNewDiskCheck()
//
//  We've done some validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned
//  to his run.  We still have a lot of verification to do.
//  This is for disk files.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      fileSetInfo:        reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:      reference to a FileCycleInfo object from MFDManager
//      pChanges:           where we store information indicating changes that need to be made in the MFD
//      pReadKeyNeeded:     we set this true if a read key exists, and was not provided
//      pWriteKeyNeeded:    we set this true if a write key exists, and was not provided
//      pReadInhibited:     we set this true if the assignment should prevent reads
//      pWriteInhibited:    we set this true if the assignment should prevent writes
//
//  Returns true if successful, else false (to stop the assign process)
bool
FacilitiesManager::asgExistingNewDiskCheck
(
    const Context&                              context,
    const UINT32                                options,
    const FileSpecification&                    fileSpecification,
    const FieldList&                            additionalFields,
    const MFDManager::FileSetInfo&              fileSetInfo,
    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo,
    NewlyAssignedChangesDisk* const             pChanges,
    bool* const                                 pReadKeyNeeded,
    bool* const                                 pWriteKeyNeeded,
    bool* const                                 pReadInhibited,
    bool* const                                 pWriteInhibited
) const
{
    *pReadKeyNeeded = false;
    *pWriteKeyNeeded = false;
    *pReadInhibited = false;
    *pWriteInhibited = false;

    //  Validate options (see 7.2.1 and 7.2.5)
    //  This is for @ASG,A{options} for disk...
    UINT32 allowedOpts = OPTB_A | OPTB_D | OPTB_E | OPTB_K | OPTB_M | OPTB_Q | OPTB_R | OPTB_X | OPTB_Y | OPTB_Z;
    if ( !checkOptions( options, allowedOpts, context.m_pResult ) )
        return false;

    //  Check for conflicting options
    UINT32 mask = OPTB_E | OPTB_Q | OPTB_Y;
    if ( !checkOptionConflicts( context, options, mask ) )
        return false;

    //  Check read/write keys
    if ( !checkKeys( context, fileSpecification, fileSetInfo, false, pReadKeyNeeded, pWriteKeyNeeded ) )
        return false;
    if ( ((options & OPTB_D) || (options & OPTB_K)) && (*pReadKeyNeeded || *pWriteKeyNeeded) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED );
        if ( *pReadKeyNeeded )
            context.m_pResult->m_StatusBitMask.logicalOr( 0400100000000ll );
        if ( *pWriteKeyNeeded )
            context.m_pResult->m_StatusBitMask.logicalOr( 0400200000000ll );
        return false;
    }

    //  Check for privacy key
    if ( !checkPrivate( context, &fileSetInfo, &fileCycleInfo ) )
        return false;

    //  Check for disablement of the file (override if run is privileged)
    //      note this coalesces with keyReadDisabled and keyWriteDisabled above...
    //  There's a special check for Y options on assign of unassigned existing file
    //      (if Y option specified, set read/write disabled, even if privileged)
    if ( !checkDisables( context, options, true, fileCycleInfo, pReadInhibited, pWriteInhibited ) )
        return false;
    if ( *pReadKeyNeeded )
        *pReadInhibited = true;
    if ( *pWriteKeyNeeded )
        *pWriteInhibited = true;
    if ( options & OPTB_Y )
    {
        *pReadInhibited = true;
        *pWriteInhibited = true;
    }

    //  Check additional subfields
    DiskSubfieldInfo diskInfo;
    if ( !checkDiskSubfields( context, additionalFields, &fileCycleInfo, 0, &diskInfo, false ) )
        return false;

    pChanges->m_UpdateInitialReserve = diskInfo.m_InitialReserveSpecified;
    pChanges->m_NewInitialReserve = diskInfo.m_InitialReserve;
    pChanges->m_UpdateMaxGranules = diskInfo.m_MaxGranulesSpecified;
    pChanges->m_NewMaxGranules = diskInfo.m_MaxGranules;

    //  Check availability - Is the file exclusively assigned (if so, it's to another run)
    if ( fileCycleInfo.m_ExclusiveUse )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_HELD_FOR_EXCLUSIVE_FILE_USE_RELEASE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0400000200000ll );
        return false;
    }

    //  Did caller specify exclusive use?  Is the file already assigned (to another run)?
    if ( (options & OPTB_X) && (fileCycleInfo.m_CurrentAssignCount > 0) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_HELD_FOR_NEED_OF_EXCLUSIVE_USE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0400001000000ll );
        return false;
    }

    //  Done
    return true;
}


//  asgExistingNewDiskExec()
//
//  We've done all validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned to his run.
//  Make it so (for disk files).
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      fileSetInfo:        reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:      reference to a FileCycleInfo object from MFDManager
//      mfdChanges:         object indicating changes need to be made in the MFD
//      readKeyNeeded:      caller did not specify required read key (this implies readInhibit == true)
//      writeKeyNeeded:     caller did not specify required write key (this implies writeInhibit == true)
//      readInhibited:      this run not allowed to read from the file
//      writeInhibited:     this run not allowed to write to the file
//
//  Returns true if successful, else false (to stop the assign process)
bool
FacilitiesManager::asgExistingNewDiskExec
(
    const Context&                              context,
    const UINT32                                options,
    const FileSpecification&                    fileSpecification,
    const FieldList&                            additionalFields,
    const MFDManager::FileSetInfo&              fileSetInfo,
    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo,
    const NewlyAssignedChangesDisk&             mfdChanges,
    const bool                                  readKeyNeeded,
    const bool                                  writeKeyNeeded,
    const bool                                  readInhibited,
    const bool                                  writeInhibited
) const
{
//    bool deleteOnAnyTermFlag =      (options & OPTB_K) != 0;  ???? why do we not look at this?
//    bool deleteOnNormalTermFlag =   (options & OPTB_D) != 0;  ???? or this?
    bool exclusiveFlag =            (options & OPTB_X) != 0;
    bool releaseFlag =              (options & OPTB_I) != 0;
//    bool temporaryFlag =            (options & OPTB_T) != 0;  ???? or this?

    //  Update MFD
    COUNT32 initialReserve = mfdChanges.m_UpdateInitialReserve ? mfdChanges.m_NewInitialReserve : fileCycleInfo.m_InitialReserve;
    COUNT32 maxGranules = mfdChanges.m_UpdateMaxGranules ? mfdChanges.m_NewMaxGranules : fileCycleInfo.m_MaxGranules;
    FileAllocationTable* pFileAllocationTable = 0;

    MFDManager::Result mfdResult = m_pMFDManager->assignFileCycle( context.m_pActivity,
                                                                   fileCycleInfo.m_DirectorySectorAddresses[0],
                                                                   exclusiveFlag,
                                                                   initialReserve,
                                                                   maxGranules,
                                                                   &pFileAllocationTable );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_ASG::asgExistingNewDiskExec", fileSpecification, mfdResult );
        return false;
    }

    //  Create FACITEM and update RunInfo
    DiskFacilityItem* pFacItem = 0;
//???? why is the following variable not used?
//    UINT16 absoluteFileCycle = fileSpecification.m_AbsoluteCycleSpecified ? fileSpecification.m_Cycle : 0;
    INT8 relativeFileCycle = fileSpecification.m_RelativeCycleSpecified ? fileSpecification.m_Cycle : 0;
    EquipmentCode equipCode = fileCycleInfo.m_WordAddressable ? ECODE_WORD_DISK : ECODE_SECTOR_DISK;

    pFacItem = new DiskFacilityItem( fileSpecification.m_FileName,
                                     fileSpecification.m_Qualifier,
                                     equipCode,
                                     options,
                                     releaseFlag,
                                     fileCycleInfo.m_AbsoluteCycle,
                                     fileSpecification.m_AbsoluteCycleSpecified,
                                     exclusiveFlag,
                                     true,              //  existing flag
                                     fileCycleInfo.m_DirectorySectorAddresses[0],
                                     readInhibited,
                                     readKeyNeeded,
                                     relativeFileCycle,
                                     fileSpecification.m_RelativeCycleSpecified,
                                     false,             //  not temporary
                                     writeInhibited,
                                     writeKeyNeeded,
                                     pFileAllocationTable,
                                     fileCycleInfo.m_PositionGranularity ? MSGRAN_POSITION : MSGRAN_TRACK,
                                     fileCycleInfo.m_HighestGranuleAssigned,
                                     fileCycleInfo.m_HighestTrackWritten,
                                     fileCycleInfo.m_InitialReserve,
                                     fileCycleInfo.m_MaxGranules );

    context.m_pRunInfo->insertFacilityItem( pFacItem );

    //  File already assigned to another run warning?
    //  We know this by seeing the assign count > 0 in the FileCycleInfo object.
    //  (in the MFD it is actually one greater than what is in fileCycleInfo,
    //  so we are actually checking whether the *new* current assign count is > 1).
    if ( fileCycleInfo.m_CurrentAssignCount > 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ASSIGNED_TO_ANOTHER_RUN );
        context.m_pResult->m_StatusBitMask.logicalOr( 0000000100000ll );
    }

    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgExistingNewTape()
//
//  We've done some validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned
//  to his run.  We still have a lot of verification to do, after which we need to
//  update the MFD a little bit, then update the user's RunInfo object as well.
//  This is specifically for cataloged tape files.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      fileSetInfo:        reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:      reference to a FileCycleInfo object created by MFDManager
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingNewTape
(
    const Context&                          context,
    const UINT32                            options,
    const FileSpecification&                fileSpecification,
    const FieldList&                        additionalFields,
    const MFDManager::FileSetInfo&          fileSetInfo,
    const MFDManager::TapeFileCycleInfo&    fileCycleInfo
) const
{
    bool readKeyNeeded  = false;
    bool writeKeyNeeded = false;
    bool readInhibited  = false;
    bool writeInhibited = false;

    UINT16              expirationPeriod        = 0;
    TapeFormat          format                  = TFMT_EIGHT_BIT_PACKED;
    char                logicalChannel          = 0;
    LSTRING             reelNumbers;
    std::vector<INDEX>  unitSelectedIndices;

    reelNumbers.push_back( "BLANK" );

    if ( !asgExistingNewTapeCheck( context,
                                   options,
                                   fileSpecification,
                                   additionalFields,
                                   fileSetInfo,
                                   fileCycleInfo,
                                   &readKeyNeeded,
                                   &writeKeyNeeded,
                                   &readInhibited,
                                   &writeInhibited,
                                   &expirationPeriod,
                                   &format,
                                   &logicalChannel,
                                   &reelNumbers,
                                   &unitSelectedIndices ) )
        return false;

    return asgExistingNewTapeExec( context,
                                   options,
                                   fileSpecification,
                                   additionalFields,
                                   fileSetInfo,
                                   fileCycleInfo,
                                   readKeyNeeded,
                                   writeKeyNeeded,
                                   readInhibited,
                                   writeInhibited,
                                   expirationPeriod,
                                   format,
                                   logicalChannel,
                                   reelNumbers,
                                   unitSelectedIndices );

    return true;
}


//  asgExistingNewTapeCheck()
//
//  We've done some validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned
//  to his run.  We still have a lot of verification to do.
//  This is for tape files.
//
//  Currently, we accept all known equipment types, and use our internal virtual tape handler regardless.
//  We therefore accept all density and parity settings, and ignore them.
//
//  Options
//      A:  cataloged file (we wouldn't be here otherwise)
//      B:  reel number, filename are non-unique across run and system
//      D:  delete file when released, or when run terminates normally
//      E:  even parity
//      F:  when labeled output tape is created, do not write qualifier and filename into label
//      H:  drives selection of device type among those allowed by equipment type
//      I:  release file at next task termination
//      J:  output tape is not to be labeled
//      K:  delete file when released, or when run terminates for any reason excluding system stop
//      L:  drives selection of device type among those allowed by equipment type
//      M:  drives selection of device type among those allowed by equipment type
//      N:  tape unit is assigned, but no reel is mounted
//      O:  odd parity
//      Q:  allow assignment even if disabled, and do not set read/write inhibits if disabled
//      R:  file is assigned as read-only
//      S:  6250bpi
//      V:  1600bpi for 9-track, 85937bpi for DLT
//      W:  file is assigned as write-only
//      X:  Do not inhibit I/O after certain block-count-check errors are detected (ignored by us)
//      Y:  Assign only for examining MFD - read inhibited, and write inhibited (even if privileged)
//      Z:  Run is not to be held if a tape unit or reel number is not available
//
//  Conflicting Options:
//      EO
//      HLMSV
//      FJ
//
//  Parameters:
//      context:                Context in which this command operates
//      options:                options given on the @ASG statement
//      fileSpecification:      the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:       additional fields beyond the file specification - the semantics of these fields and
//                                  subfields are dependent upon (to some extent) the options, and more directly,
//                                  to the equipment type of the file involved
//      fileSetInfo:            reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:          reference to a FileCycleInfo object from MFDManager
//      pReadKeyNeeded:         we set this true if a read key exists, and was not provided
//      pWriteKeyNeeded:        we set this true if a write key exists, and was not provided
//      pReadInhibited:         we set this true if the assignment should prevent reads
//      pWriteInhibited:        we set this true if the assignment should prevent writes
//      pFormat:                where we store the selected tape format (caller should init this to default)
//      pLogicalChannel:        where we store the logical channel selection, if any (caller should init this to zero)
//      pReelNumbers:           where we store the specified reel number(s) if any
//      pUnitSelectedIndices:   where we store the m_Device[] index[ices] for the unit[s] we have selected
//
//  Returns true if successful, else false (to stop the assign process)
bool
FacilitiesManager::asgExistingNewTapeCheck
(
    const Context&                          context,
    const UINT32                            options,
    const FileSpecification&                fileSpecification,
    const FieldList&                        additionalFields,
    const MFDManager::FileSetInfo&          fileSetInfo,
    const MFDManager::TapeFileCycleInfo&    fileCycleInfo,
    bool* const                             pReadKeyNeeded,
    bool* const                             pWriteKeyNeeded,
    bool* const                             pReadInhibited,
    bool* const                             pWriteInhibited,
    UINT16* const                           pExpirationPeriod,
    TapeFormat* const                       pFormat,
    char* const                             pLogicalChannel,
    LSTRING* const                          pReelNumbers,
    std::vector<INDEX>* const               pUnitSelectedIndices
) const
{
    *pReadKeyNeeded = false;
    *pWriteKeyNeeded = false;
    *pReadInhibited = false;
    *pWriteInhibited = false;

//????    bool    noiseConstantSpecified  = false;
//????    COUNT   unitCount               = 0;

    //  Validate options (see 7.2.1 and 7.2.5)
    //  This is for @ASG,A{options} for tape...
    UINT32 allowedOpts = OPTB_A | OPTB_D | OPTB_K | OPTB_Q | OPTB_X | OPTB_Y
                            | OPTB_B | OPTB_I | OPTB_N | OPTB_R | OPTB_W | OPTB_Z
                            | OPTB_E | OPTB_H | OPTB_L | OPTB_M | OPTB_O | OPTB_S | OPTB_V
                            | OPTB_F | OPTB_J;
    if ( !checkOptions( options, allowedOpts, context.m_pResult ) )
        return false;

    //  Check mutually exclusive option conflicts...
    if ( !checkOptionConflicts( context, options, OPTB_Q | OPTB_Y ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_E | OPTB_O ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_H | OPTB_L | OPTB_M | OPTB_S | OPTB_V ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_F | OPTB_J ) )
        return false;

    //  Check read/write keys
    if ( !checkKeys( context, fileSpecification, fileSetInfo, false, pReadKeyNeeded, pWriteKeyNeeded ) )
        return false;
    if ( ((options & OPTB_D) || (options & OPTB_K)) && (*pReadKeyNeeded || *pWriteKeyNeeded) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED );
        if ( *pReadKeyNeeded )
            context.m_pResult->m_StatusBitMask.logicalOr( 0400100000000ll );
        if ( *pWriteKeyNeeded )
            context.m_pResult->m_StatusBitMask.logicalOr( 0400200000000ll );
        return false;
    }

    //  Check for privacy key
    if ( !checkPrivate( context, &fileSetInfo, &fileCycleInfo ) )
        return false;

    //  Check for disablement of the file (override if run is privileged)
    //      note this coalesces with keyReadDisabled and keyWriteDisabled above...
    if ( !checkDisables( context, options, true, fileCycleInfo, pReadInhibited, pWriteInhibited ) )
        return false;
    if ( *pReadKeyNeeded )
        *pReadInhibited = true;
    if ( *pWriteKeyNeeded )
        *pWriteInhibited = true;

#if 0 //TODO:TAPE use checkTapeSubfields()
    //  Check additional fields and subfields
    //  Per PRM:
    //      type[/units/log/noise/processor/tape/format/data-converter/block-numbering/data-compression/buffered-write/expanded-buffer
    //      ,reel-1/reel-2/.../reel-n
    //      ,expiration/mmspec
    //      ,ring-indicator
    //      ,ACR-name
    //      ,CTL-pool ]
    //  Per Implementation:
    //      type[/units/log/noise///format,reel-1/reel-2/.../reel-n,expiration,ring-indicator
    //  units is accepted and validated, but has no effect - must be 1 or 2
    //  log is accepted and validated, but has no effect - must be a single alphabetical letter
    //  noise is accepted and validated, but has no effect - must be 1 to 63
    //  processor subfield is not accepted
    //  tape subfield is not accepted
    //  data-converter is not accepted
    //  block-numbering is not accepted
    //  data-compression is not accepted
    //  buffered-write is not accepted
    //  expanded-buffer is not accepted
    //  mmspec is not accepted
    //  ACR-name is not accepted
    //  CTL-pool is not acccepted
    if ( additionalFields.size() > 0 )
    {
        //  Is there anything in the first field?
        if ( additionalFields[0].size() )
        {
            const EquipmentType* pSpecifiedEquipType = 0;
            std::string reportedMnem = fileCycleInfo.m_AssignMnemonic;

            //  subfield 0: Make sure type is compatible with the cataloged file
            if ( (additionalFields[0].size() > 0) && (additionalFields[0][0].size() > 0) )
            {
                SuperString mnemonic = additionalFields[0][0];
                mnemonic.foldToUpperCase();
                pSpecifiedEquipType = checkEquipmentType( mnemonic, pResult );
                if ( pSpecifiedEquipType == 0 )
                    return false;

                if ( pSpecifiedEquipType->m_EquipmentCategory != ECAT_TAPE )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_GENERIC_TYPE );
                    context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
                    return false;
                }

                reportedMnem = pSpecifiedEquipType->m_pMnemonic;
            }

            //  subfield 1: Number of units (1 or 2)
            if ( (additionalFields[0].size() > 1) && (additionalFields[0][1].size() > 0) )
            {
                SuperString str = additionalFields[0][1];
                bool good = false;
                if ( str.size() == 1 )
                {
                    if ( str[0] == '1' )
                    {
                        unitCount = 1;
                        good = true;
                    }
                    else if ( str[0] == '2' )
                    {
                        unitCount = 2;
                        good = true;
                    }
                }

                if ( !good )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_FOR_NUMBER_OF_UNITS );
                    context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                    return false;
                }
            }

            //  subfield 2: logical channel (alphabetic character)
            if ( (additionalFields[0].size() > 2) && (additionalFields[0][2].size() > 0) )
            {
                if ( (additionalFields[0][2].size() > 1) || !isalpha(additionalFields[0][2][0]) )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_LOGICAL_CHANNEL );
                    context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                    return false;
                }

                *pLogicalChannel = additionalFields[0][2][0];
                if ( islower( *pLogicalChannel ) )
                    *pLogicalChannel = toupper( *pLogicalChannel );
            }

            //  subfield 3: noise (integer 1-63)
            if ( (additionalFields[0].size() > 3) && (additionalFields[0][3].size() > 0) )
            {
                SuperString str = additionalFields[0][3];
                bool good = false;
                if ( str.isDecimalNumeric() )
                {
                    COUNT nc = str.toDecimal();
                    if ( (nc > 0) && (nc < 64) )
                        good = true;
                }

                if ( !good )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_NOISE_CONSTANT_VALUE );
                    context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                    return false;
                }

                //  For now, we only have one tape type, and it noise constant doesn't apply.
                //  If we ever support a real tape device which *does* use a noise constant,
                //  we'll have to rethink this...
                context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_MNEMONIC_DOES_NOT_ALLOW_NOISE_FIELD, reportedMnem ) );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                return false;
            }

            //  subfield 6: format ('Q' or '8' or '6')
            if ( (additionalFields[0].size() > 6) && (additionalFields[0][6].size() > 0) )
            {
                bool okay = false;
                if (additionalFields[0][6].size() == 1)
                {
                    switch ( additionalFields[0][6][0] )
                    {
                    case 'Q':
                    case 'q':
                        *pFormat = TFMT_QUARTER_WORD;
                        okay = true;
                        break;

                    case '8':
                        *pFormat = TFMT_EIGHT_BIT_PACKED;
                        okay = true;
                        break;

                    case '6':
                        context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ASSIGN_MNEMONIC_DOES_NOT_SUPPORT_6_BIT_PACKED, reportedMnem ) );
                        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                        return false;
                    }
                }

                if ( !okay )
                {
                    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_FORMAT );
                    context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                    return false;
                }
            }

            //  Any illegal subfields specified?
            if ( ((additionalFields[0].size() >= 5) && (additionalFields[0][4].size() > 0)) 
                || ((additionalFields[0].size() >= 6) && (additionalFields[0][5].size() > 0))
                || (additionalFields[0].size() > 7) )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
                return false;
            }
        }

        //  Anything in the second field?  If so, it is a list of reel numbers
        if ( (additionalFields.size() >= 2) && (additionalFields[1].size() > 0) )
        {
            // reel numbers already in MFD, what are we doing here?
            pReelNumbers->clear();

            //
            //E:202533 reel nnnnnn already in use by this run.
            //E:202633 reel nnnnnn is not in the master file directory.
            //E:241333 Blank reel-id must be alone.
            //E:247733 Maximum number of reelids exceeded.
            //E:253533 Number of reelids on image greater than number of reelids in master file directory.
            //E:254233 Scrtch is not allowed on a cataloged file.
            //Bit 15* invalid or duplicate reel number... on @ASG statement, req reel already assigned by this run
        }

        //  Anything in the third field?  If so, subfield 0 is is the expiration value
        if ( (additionalFields.size() >= 3) && (additionalFields[2].size() > 0) )
        {
            //
            //  E:244133 Expiration exceeds the configured maximum.
            //  E:244233 Illegal value specified for expiration period.
        }

        //  fourth field?  If so, subfield 0 is the RING/NORING indicator
        if ( (additionalFields.size() >= 4) && (additionalFields[3].size() > 0) )
        {
            //
            //E:250533 Noring cannot be specified with SCRTCH or BLANK reelid.
            //E:254133 Illegal value for ring specification
        }

        //  Check for too many fields
        if ( additionalFields[0].size() > 4)
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return false;
        }
    }
#endif

#if 0 //TODO:TAPE
    //  Check for option/equipment type conflicts
    //  E:251033 Options M and L are not allowed with 9 track tape units.
    //  E:251133 Options V and S are not allowed with 7 track tape units.
    //  E:204533 Assign mnemonic {asg-mnem] requires the J option.
    //  E:204733 Assign mnemonic {asg-mnem] does not support M-L-H-V density options.
    //  E:206333 Assign mnemonic {asg-mnem] does not support M-L-V-S density options.

    //  Can we actually do this?  If not, complain.  If so, select unit(s).
    //  First, count the number of units we have, regardless of whether they are currently available.
    // need to interact with DeviceManager for this
    COUNT unitsConfigured = 0;
    for ( INDEX ux = 0; ux < m_Devices.size(); ++ux )
        if ( m_Devices[ux]->m_EquipmentModel.m_EquipmentCategory == ECAT_TAPE )
            ++unitsConfigured;

    if ( unitsConfigured == 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_NO_TAPE_UNITS_CONFIGURED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }
    else if ( unitsConfigured < unitCount )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_INSUFFICIENT_NUMBER_OF_UNITS_AVAILABLE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }

    //  Now find compatible units - they must be generally compatible with the given EquipmentType,
    //  and they must be exactly compatible with each other.

    //  Check availability - Is the file assigned elsewhere (if so, it's to another run)
    //  Don't do this for option B.
    //  E:257033 Hold for reel rejected because of Z option.
    if ( fileCycleInfo.m_ExclusiveUse )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_HELD_FOR_EXCLUSIVE_FILE_USE_RELEASE );//?
        context.m_pResult->m_StatusBitMask.logicalOr( 0400000200000ll );//?
        return false;
    }

    //  If not specified, select system-default noise constant based on the equipment type.
    //  The system standard value is 3 for all 7-track tapes and 9-track tapes
    //  written in a density of 800 bpi. Any 9-track tapes written in a density of 1600 bpi
    //  and 6250 bpi have 0 as the system standard value.

    //  Done
#endif
    return true;
}


//  asgExistingNewTapeExec()
//
//  We've done all validation, and determined that the caller is attempting to assign
//  a file cycle (either via absolute or relative f-cycle) which is not yet assigned to his run.
//  Make it so (for disk files).
//
//  Parameters:
//      context:                Context in which this command operates
//      options:                options given on the @ASG statement
//      fileSpecification:      the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:       additional fields beyond the file specification - the semantics of these fields and
//                                  subfields are dependent upon (to some extent) the options, and more directly,
//                                  to the equipment type of the file involved
//      fileSetInfo:            reference to a FileSetInfo object created by MFDManager
//      fileCycleInfo:          reference to a FileCycleInfo object from MFDManager
//      readKeyNeeded:          caller did not specify required read key (this implies readInhibit == true)
//      writeKeyNeeded:         caller did not specify required write key (this implies writeInhibit == true)
//      readInhibited:          this run not allowed to read from the file
//      writeInhibited:         this run not allowed to write to the file
//      currentReelNumber:
//      expirationPeriod:       value for expiration period
//      logicalChannel:         ignored
//      nextReelNumber:
//      totalReelCount:
//      unitSelectedIndices:    m_Device indices of the units which we've selected
//
//  Returns true if successful, else false (to stop the assign process)
bool
FacilitiesManager::asgExistingNewTapeExec
(
    const Context&                          context,
    const UINT32                            options,
    const FileSpecification&                fileSpecification,
    const FieldList&                        additionalFields,
    const MFDManager::FileSetInfo&          fileSetInfo,
    const MFDManager::TapeFileCycleInfo&    fileCycleInfo,
    const bool                              readKeyNeeded,
    const bool                              writeKeyNeeded,
    const bool                              readInhibited,
    const bool                              writeInhibited,
    const UINT16                            expirationPeriod,
    const TapeFormat                        format,
    const char                              logicalChannel,
    const LSTRING&                          reelNumbers,
    const std::vector<INDEX>&               unitSelectedIndices
) const
{
    /*????
    bool deleteOnAnyTermFlag =      (options & OPTB_K) != 0;
    bool deleteOnNormalTermFlag =   (options & OPTB_D) != 0;
    bool exclusiveFlag =            (options & OPTB_X) != 0;
    bool releaseFlag =              (options & OPTB_I) != 0;
    bool temporaryFlag =            (options & OPTB_T) != 0;
     */

    //  Create FACITEM and update RunInfo
#if 0 //TODO:TAPE some wrongness here
    FacilityItem* pFacItem = 0;
    UINT16 absoluteFileCycle = fileSpecification.m_AbsoluteCycleSpecified ? fileSpecification.m_Cycle : 0;
    INT8 relativeFileCycle = fileSpecification.m_RelativeCycleSpecified ? fileSpecification.m_Cycle : 0;
    EquipmentCode eqcode = m_Devices[unitSelectedIndices[0]]->m_EquipmentModel.m_EquipmentCode;

    pFacItem = new TapeFacilityItem( fileSpecification.m_FileName,
                                        fileSpecification.m_Qualifier,
                                        eqcode,
                                        options,
                                        releaseFlag,
                                        temporaryFlag,
                                        absoluteFileCycle,
                                        fileSpecification.m_AbsoluteCycleSpecified,
                                        deleteOnAnyTermFlag,
                                        deleteOnNormalTermFlag,
                                        exclusiveFlag,
                                        fileId,
                                        readInhibited,
                                        readKeyNeeded,
                                        relativeFileCycle,
                                        fileSpecification.m_RelativeCycleSpecified,
                                        writeInhibited,
                                        writeKeyNeeded,
                                        currentReelNumber,
                                        expirationPeriod,
                                        logicalChannel,
                                        nextReelNumber,
                                        0,                  //  we don't do noise constants for now
                                        totalReelCount,
                                        unitSelectedIndices.size() );

    pRunInfo->insertFacilityItem( pFacItem );
#endif

    //  File already assigned to another run warning?
    //  We know this by seeing the assign count > 0 in the FileCycleInfo object.
    //  (in the MFD it is actually one greater than what is in fileCycleInfo,
    //  so we are actually checking whether the *new* current assign count is > 1).
    if ( fileCycleInfo.m_CurrentAssignCount > 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ASSIGNED_TO_ANOTHER_RUN );
        context.m_pResult->m_StatusBitMask.logicalOr( 0000000100000ll );
    }

    //  Done
    return true;//    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgExistingRelative()
//
//  Attempts to assign the indicated file, using the indicated relative file cycle
//  and assuming relative file cycle 0 if none was specified.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false
bool
FacilitiesManager::asgExistingRelative
(
    const Context&                  context,
    const UINT32                    options,
    const FileSpecification&        fileSpecification,
    const FieldList&                additionalFields
) const
{
    //  Ensure relative f-cycle is acceptable
    if ( fileSpecification.m_Cycle > 0 )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FCYCLE_PLUS_ONE_ILLEGAL_WITH_A );
        context.m_pResult->m_StatusBitMask.setW( 0400000000040 );
        return false;
    }

    //  First, search assigned files to see if the requested relative F-cycle is already
    //  assigned to the run.  If so, go to alreadyAssigned code.
    CITFACITEMS itFacItem;
    if ( isFileAssigned( fileSpecification, *context.m_pRunInfo, &itFacItem ) )
        return asgExistingAlreadyAssigned( context, options, fileSpecification, additionalFields, itFacItem );

    //  Search MFD to see whether the relative F-cycle exists.  If not, reject the assignment.
    MFDManager::FileSetInfo fileSetInfo;
    INDEX fileSetCycleEntryIndex;

    if ( !findLeadItemInfo( fileSpecification, &fileSetInfo, &fileSetCycleEntryIndex ) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_NOT_CATALOGED );
        context.m_pResult->m_StatusBitMask.setW( 0400010000000 );
        return false;
    }

    //  Find F-cycle's absolute cycle number, and check to see whether that F-cycle is
    //  already assigned to the run.
    FileSpecification absSpecification = fileSpecification;
    absSpecification.m_AbsoluteCycleSpecified = true;
    absSpecification.m_RelativeCycleSpecified = false;
    absSpecification.m_Cycle = fileSetInfo.m_CycleEntries[fileSetCycleEntryIndex].m_AbsoluteCycle;

    if ( isFileAssigned( absSpecification, *context.m_pRunInfo, &itFacItem ) )
    {
        StandardFacilityItem* pFacItem = dynamic_cast<StandardFacilityItem*>( itFacItem->second );

        //  F-cycle already assigned to the run.
        //  If it was assigned via a (different) relative F-cycle, reject due to naming conflict.
        if ( pFacItem->getRelativeFileCycleSpecified() )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_RELATIVE_FCYCLE_CONFLICT );
            context.m_pResult->m_StatusBitMask.setW( 0400000000040 );
            return false;
        }

        //  No relative f-cycle is associated with the assigned F-cycle.
        //  Call common already-assigned code, and if that works, associate the existing item
        //  with the newly-requested relative F-cycle.
        bool asgResult = asgExistingAlreadyAssigned( context, options, fileSpecification, additionalFields, itFacItem );
        if ( asgResult )
        {
            pFacItem->setRelativeFileCycleSpecified( true );
            pFacItem->setRelativeFileCycle( static_cast<INT8>(fileSpecification.m_Cycle) );
        }

        return asgResult;
    }

    //  Assign the F-cycle to the run.
    return asgExistingNew( context, options, fileSpecification, additionalFields, fileSetInfo, fileSetCycleEntryIndex );
}


//  asgPostProcessing()
//
//  Post processing for all @ASG functionality
bool
FacilitiesManager::asgPostProcessing
(
    const Context&              context,
    const FileSpecification&    fileSpecification,
    FacilityItem* const         pFacilityItem
) const
{
    DiskFacilityItem* pdfi = dynamic_cast<DiskFacilityItem*>( pFacilityItem );
    StandardFacilityItem* psfi = dynamic_cast<StandardFacilityItem*>( pFacilityItem );
    bool result = true;

    //  Resolve any appropriate orphan NameItems
    if ( psfi )
        asgResolveNameItems( context, psfi );

    //  Is this a successful uninhibited @ASG of (disk file) DLOC$?
    if ( pdfi )
        asgCheckForDLOC$( context, fileSpecification, pdfi );

    //  If this is disk, allocate initial reserve
    if ( pdfi )
        result = asgAllocateInitialReserve( context, pdfi );

    return result;
}


//  asgTemporary()
//
//  Used for validating the various fields and options for assigning a temporary disk or tape file.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false... if anyone cares.
bool
FacilitiesManager::asgTemporary
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields
) const
{
    //  Is the file already assigned to the run?
    CITFACITEMS itFacItem;
    if ( checkTempFileAssigned( context, effectiveSpec, &itFacItem ) )
    {
        if ( context.m_pResult->containsErrorStatusCode() )
            return false;
        return asgTemporaryAlreadyAssigned( context, options, effectiveSpec, additionalFields, itFacItem );
    }

    //  This is a new assignment.
    return asgTemporaryNew( context, options, effectiveSpec, additionalFields );
}


//  asgTemporaryAlreadyAssigned()
//
//  Extension of asgTemporary() where we have determined that the file is already assigned.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      itFacItem:          iterator into m_FacilityItems indicating the assigned file entry
//
//  Returns true if successful, else false... if anyone cares.
inline bool
FacilitiesManager::asgTemporaryAlreadyAssigned
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields,
    const CITFACITEMS           itFacItem
) const
{
    //  Branch based on the generic equipment type of the already-assigned file
    if ( itFacItem->second->isSectorMassStorage() || itFacItem->second->isWordMassStorage() )
        return asgTemporaryAlreadyAssignedDisk( context, options, effectiveSpec, additionalFields, itFacItem );
    if ( itFacItem->second->isTape() )
        return asgTemporaryAlreadyAssignedTape( context, options, effectiveSpec, additionalFields, itFacItem );
    //  TODO::ABSASG

    //  At this point, the already-assigned file is nothing we expected.  This is Not Good.
    m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
    return false;
}


//  asgTemporaryAlreadyAssignedDisk()
//
//  Used for validating the various fields and options for assigning a temporary disk file.
//  We assume that the T option was given or implied.
//  The file has been determined to already be assigned, so we just make sure the caller didn't
//  specify anything too badly conflicting.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//                              I: Automatically free the file at the next task termination, regardless of how it terminates
//                              T: expected, but might not be there
//                              Z: Do not allow a hold condition.  This probably has no effect here.
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      pEquipType:         pointer to relevant EquipType struct
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pFacItem:           pointer to the FacItem for the already-assigned file
//
//  Returns true if successful, else false... if anyone cares.
bool
FacilitiesManager::asgTemporaryAlreadyAssignedDisk
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS           itFacItem
) const
{
    //  Verify validity of options specified.
    //  This is insufficient for the case where options associated with cataloged files are given in the absence
    //  of A,C,T, or U options, and the file does not exist (implying T option).
    //  In this case, additional options should be ignored.  But we fail if they're there.  Too bad.
    if ( !checkOptions( options, OPTB_I | OPTB_T | OPTB_Z, context.m_pResult ) )
        return false;

    //  Check disk subfields...
    DiskSubfieldInfo diskInfo;
    if ( !checkDiskSubfields( context, additionalFields, 0, itFacItem->second, &diskInfo, false ) )
        return false;

    //  Update existing FacilityItem object in the RunInfo object.
    DiskFacilityItem* pDiskItem = dynamic_cast<DiskFacilityItem*>( itFacItem->second );

    //  Set flag to automatically release the run next time a task terminates? (I option)
    if ( options & OPTB_I )
        pDiskItem->setReleaseFlag( true );

    //  (maybe) update intial reserve and max
    if ( diskInfo.m_InitialReserveSpecified )
        pDiskItem->setInitialGranules( diskInfo.m_InitialReserve );
    if ( diskInfo.m_MaxGranulesSpecified )
        pDiskItem->setMaximumGranules( diskInfo.m_MaxGranules );

    //  File is (obviously) already assigned
    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

    return asgPostProcessing( context, fileSpecification, pDiskItem );
}


//  asgTemporaryAlreadyAssignedTape()
//
//  Used for validating the various fields and options for assigning a temporary tape file.
//  We assume that the T option was given or implied.
//  The file has been determined to already be assigned, so we just make sure the caller didn't
//  specify anything too badly conflicting.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//                              I: Automatically free the file at the next task termination, regardless of how it terminates
//                              T: expected, but might not be there
//                              Z: Do not allow a hold condition.  This probably has no effect here.
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      pEquipType:         pointer to relevant EquipType struct
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pFacItem:           pointer to the FacItem for the already-assigned file
//
//  Returns true if successful, else false... if anyone cares.
bool
FacilitiesManager::asgTemporaryAlreadyAssignedTape
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields,
    const CITFACITEMS           itFacItem
) const
{
    //  Verify validity of options specified.
    UINT32 allowed = OPTB_B | OPTB_I | OPTB_N | OPTB_R | OPTB_W | OPTB_Z
                    | OPTB_R | OPTB_T | OPTB_W
                    | OPTB_E | OPTB_H | OPTB_L | OPTB_M | OPTB_O | OPTB_S | OPTB_V
                    | OPTB_F | OPTB_J;
    if ( !checkOptions( options, allowed, context.m_pResult ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_E | OPTB_O ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_H | OPTB_L | OPTB_M | OPTB_S | OPTB_V ) )
        return false;
    if ( !checkOptionConflicts( context, options, OPTB_F | OPTB_J ) )
        return false;

    //  Check subfields...
    TapeSubfieldInfo tapeInfo;
    if ( !checkTapeSubfields( context, additionalFields, 0, 0, itFacItem->second, &tapeInfo ) )
        return false;

    //  Update existing FacilityItem object in the RunInfo object.
    TapeFacilityItem* pFacItem = dynamic_cast<TapeFacilityItem*>( itFacItem->second );

    //  Set flag to automatically release the run next time a task terminates? (I option)
    if ( options & OPTB_I )
        pFacItem->setReleaseFlag( true );

    //  File is (obviously) already assigned
    context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
    context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgTemporaryNew()
//
//  Extension of asgTemporary() where we have determined that the file has not already been assigned.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false... if anyone cares.
inline bool
FacilitiesManager::asgTemporaryNew
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields
) const
{
    //  Disk or tape?  Use equipment field to decide.
    //  Blank asgmnemonic (eqptype) indicates sector-addressable mass storage.
    const EquipmentType* pEquipType = checkEquipmentType( context, additionalFields );
    if ( pEquipType == 0 )
    {
        if ( context.m_pResult->containsErrorStatusCode() )
            return false;
        pEquipType = m_pDefaultEquipMassStorage;
    }

    //  For now, equip type will only be DISK or TAPE
    //  TODO:ABSASG - absolute assign might still be disk or tape, what to do here?
    if ( pEquipType->m_EquipmentCategory == ECAT_DISK )
        return asgTemporaryNewDisk( context, options, effectiveSpec, additionalFields, pEquipType );
    if ( pEquipType->m_EquipmentCategory == ECAT_TAPE )
        return asgTemporaryNewTape( context, options, effectiveSpec, additionalFields, pEquipType );

    m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
    return false;
}


//  asgTemporaryNewDisk()
//
//  Used for validating the various fields and options for assigning a temporary disk file.
//  We assume that the T option was given or implied.  The file has not been previously assigned.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//                              I: Automatically free the file at the next task termination, regardless of how it terminates
//                              T: expected, but might not be there
//                              Z: Do not allow a hold condition.  This probably has no effect here.
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pEquipType:         pointer to relevant EquipType struct
//
//  Returns true if successful, else false... if anyone cares.
bool
FacilitiesManager::asgTemporaryNewDisk
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields,
    const EquipmentType* const  pEquipType
) const
{
    //  Verify validity of options specified.
    //  This is insufficient for the case where options associated with cataloged files are given in the absence
    //  of A,C,T, or U options, and the file does not exist (implying T option).
    //  In this case, additional options should be ignored.  But we fail if they're there.  Too bad.
    if ( !checkOptions( options, OPTB_I | OPTB_T | OPTB_Z, context.m_pResult ) )
        return false;

    //  Check disk subfields...
    DiskSubfieldInfo diskInfo;
    diskInfo.m_pEquipmentType = pEquipType;
    if ( !checkDiskSubfields( context, additionalFields, 0, 0, &diskInfo, false ) )
        return false;

    UINT16  absoluteCycle =             effectiveSpec.m_AbsoluteCycleSpecified ? effectiveSpec.m_Cycle : 0;
    bool    absoluteCycleSpecified =    effectiveSpec.m_AbsoluteCycleSpecified;
//    bool    deleteOnAnyTermFlag =       (options & OPTB_K) != 0; ???? why not used
//    bool    deleteOnNormalTermFlag =    (options & OPTB_D) != 0; ???? ditto
    INT8    relativeCycle =             effectiveSpec.m_RelativeCycleSpecified ? effectiveSpec.m_Cycle : 0;
    bool    relativeCycleSpecified =    effectiveSpec.m_RelativeCycleSpecified;
    bool    releaseFlag =               (options & OPTB_I) != 0;
//    bool    temporaryFlag =             (options & OPTB_T) != 0; ???? why not used

    //  Create and store an appropriate FacilityItem object in the RunInfo object.
    MassStorageGranularity granularity = diskInfo.m_GranularitySpecified ? diskInfo.m_Granularity : MSGRAN_TRACK;
    COUNT32 initialGranules = diskInfo.m_InitialReserveSpecified ? diskInfo.m_InitialReserve : 0;
    COUNT32 maximumGranules = diskInfo.m_MaxGranulesSpecified ? diskInfo.m_MaxGranules : m_DefaultMaxGranules;
    EquipTypeGroup etGroup = diskInfo.m_pEquipmentType->m_EquipmentTypeGroup;
    EquipmentCode equipCode = etGroup == FACETG_SECTOR_DISK ? ECODE_SECTOR_DISK : ECODE_WORD_DISK;
    FileAllocationTable* pFileAllocationTable = new FileAllocationTable( 0, false );    //TODO:REM

    DiskFacilityItem* pFacItem = new DiskFacilityItem( effectiveSpec.m_FileName,
                                                       effectiveSpec.m_Qualifier,
                                                       equipCode,
                                                       options,
                                                       releaseFlag,
                                                       absoluteCycle,
                                                       absoluteCycleSpecified,
                                                       true,                   //  always exclusive
                                                       false,                  //  not existing
                                                       0,                      //  No DSADDR for temporary files
                                                       false,                  //  never read-inhibited
                                                       false,                  //  read key is never needed
                                                       relativeCycle,
                                                       relativeCycleSpecified,
                                                       true,                   //  temporary file
                                                       false,                  //  never write-inhibited
                                                       false,                  //  write key is never needed
                                                       pFileAllocationTable,
                                                       granularity,
                                                       0,                      //  highest granule assigned
                                                       0,                      //  highest track written
                                                       initialGranules,
                                                       maximumGranules );

    context.m_pRunInfo->insertFacilityItem( pFacItem );
    return asgPostProcessing( context, effectiveSpec, pFacItem );
}


//  asgTemporaryNewTape()
//
//  Used for validating the various fields and options for assigning a temporary tape file.
//  We assume that the T option was given or implied.  The file has not been previously assigned.
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//                              I: Automatically free the file at the next task termination, regardless of how it terminates
//                              T: expected, but might not be there
//                              Z: Do not allow a hold condition.  This probably has no effect here.
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//      pEquipType:         pointer to relevant EquipType struct
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgTemporaryNewTape
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields,
    const EquipmentType* const  pEquipType
) const
{
    //  Verify validity of options specified.
    if ( !checkOptions( options, OPTB_I | OPTB_T | OPTB_Z, context.m_pResult ) )
        return false;

    //  Check tape subfields...
    TapeSubfieldInfo tapeInfo;
    tapeInfo.m_pEquipmentType = pEquipType;
    if ( !checkTapeSubfields( context, additionalFields, 0, 0, 0, &tapeInfo ) )
        return false;

#if 0 //TODO:TAPE
    UINT16  absoluteCycle =             effectiveSpec.m_AbsoluteCycleSpecified ? effectiveSpec.m_Cycle : 0;
    bool    absoluteCycleSpecified =    effectiveSpec.m_AbsoluteCycleSpecified;
    bool    deleteOnAnyTermFlag =       (options & OPTB_K) != 0;
    bool    deleteOnNormalTermFlag =    (options & OPTB_D) != 0;
    bool    exclusiveFlag =             (options & OPTB_X) != 0;
    bool    releaseFlag =               (options & OPTB_I) != 0;
    bool    temporaryFlag =             (options & OPTB_T) != 0;

    //  Get tape unit(s)
    //????

    //  Create and store an appropriate FacilityItem object in the RunInfo object.
    FacilityItem* pNewFacItem = new TapeFacilityItem( effectiveSpec.m_FileName,
                                                        effectiveSpec.m_Qualifier,
                                                        pEquipType->m_pMnemonic,
                                                        options,
                                                        releaseFlag,
                                                        true,                   //  temporary flag is always true
                                                        absoluteCycleSpecified,
                                                        absoluteCycle,
                                                        deleteOnAnyTermFlag,
                                                        deleteOnNormalTermFlag,
                                                        true,
                                                        0,                      //  not cataloged, so no MFD identifier
                                                        false,                  //  never read-inhibited
                                                        false,                  //  read key is never needed
                                                        false,                  //  never write-inhibited
                                                        false,                  //  write key is never needed
                                                        ECODE_SECTOR_DISK,
                                                        granularity,
                                                        0,                      //  highest granule assigned
                                                        0,                      //  highest track referenced
                                                        initialGranules,
                                                        maximumGranules );
    pRunInfo->insertFacilityItem( pNewFacItem );
#endif

    return true;//    return asgPostProcessing( context, fileSpecification, pFacItem );
}


//  asgUnspecified()
//
//  Used in the case where @ASG was specified with none of the A,C,U, or T options specified.
//  We poke around in RunInfo and maybe the MFD to figure out what the caller really wants to do.
//  The algorithm is as follows:
//      If the file is already assigned, assume whichever of A,C,T, or U were on the original @ASG
//      -else-
//      If the file is cataloged, assume the A option is given.
//      -else-
//      Assume T option
//  The assumed option is masked into the original options, with the result being propogated downward
//
//  Parameters:
//      context:            Context in which this command operates
//      options:            options given on the @ASG statement
//      effectiveSpec:      the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved
//
//  Returns true if successful, else false.
bool
FacilitiesManager::asgUnspecified
(
    const Context&              context,
    const UINT32                options,
    const FileSpecification&    effectiveSpec,
    const FieldList&            additionalFields
) const
{
    UINT32 newOptions = options;

    //  Is the file currently assigned?  This code is similar to @USE code which does the same check...
    //  However it is *not* the same, as we can match any arbitrary facility item, not just standard ones.
    CITFACITEMS itfi;
    if ( isFileAssigned( effectiveSpec, *context.m_pRunInfo, &itfi ) )
    {
        //  Determine whether it was assigned using the A, C, T, or U option (exactly one of these will be set),
        //  set that option into the newOptions value, and call asgExisting() to continue.
        UINT32 actuOpt = itfi->second->getAssignOptions() & (OPTB_A | OPTB_C | OPTB_T | OPTB_U);
        newOptions |= actuOpt;
        return asgExisting( context, options, effectiveSpec, additionalFields  );
    }

    //  File is not assigned.  Is the file cataloged?
    MFDManager::FileCycleInfo fileCycleInfo;
    MFDManager::FileSetInfo fileSetInfo;
    MFDManager::Result mfdResult = m_pMFDManager->getFileSetInfo( effectiveSpec.m_Qualifier,
                                                                  effectiveSpec.m_FileName,
                                                                  &fileSetInfo );
    if ( mfdResult.m_Status == MFDManager::MFDST_SUCCESSFUL )
    {
        INDEX ex;
        if ( findExistingCycleEntryIndex( effectiveSpec, fileSetInfo, &ex ) )
        {
            //  It is cataloged (and we know it is not assigned).  Assume A option and proceed
            newOptions |= OPTB_A;
            return asgExistingNew( context, newOptions, effectiveSpec, additionalFields, fileSetInfo, ex );
        }
    }

    //  File is neither assigned nor cataloged.  Assume user wants a temporary file.
    //  Use the equipment type field to figure out whether he wants a disk or a tape file.
    //  If the caller didn't specify anything, assume sector mass storage.
    newOptions |= OPTB_T;
    const EquipmentType* pEquipType = checkEquipmentType( context, additionalFields );
    if ( !pEquipType )
    {
        if ( context.m_pResult->containsErrorStatusCode() )
            return false;
        pEquipType = m_pDefaultEquipMassStorage;
    }

    if ( pEquipType->m_EquipmentCategory == ECAT_DISK )
        return asgTemporaryNewDisk( context, newOptions, effectiveSpec, additionalFields, pEquipType );
    if ( pEquipType->m_EquipmentCategory == ECAT_TAPE )
        return asgTemporaryNewTape( context, newOptions, effectiveSpec, additionalFields, pEquipType );

    //  should not be able to get here; the only equipment types we have are disk and tape
    m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
    std::string logMsg = "FacilitiesManager_ASG::asgUnspecified():Problem with equipment type:";
    logMsg += pEquipType->m_pMnemonic;
    SystemLog::write( logMsg );

    return false;
}



//  private static methods

//  asgCheckForDLOC$()
//
//  Did we just assign SYS$*DLOC$ successfully?  If so, set privileged mode. 
void
FacilitiesManager::asgCheckForDLOC$
(
    const Context&                      context,
    const FileSpecification&            fileSpecification,
    const StandardFacilityItem* const   pFacItem
)
{
    if ( (fileSpecification.m_Qualifier.compareNoCase( "SYS$" ) == 0)
        && (fileSpecification.m_FileName.compareNoCase( "DLOC$" ) == 0)
        && !pFacItem->isTape()
        && !pFacItem->getTemporaryFileFlag()
        && !pFacItem->getReadInhibitedFlag()
        && !pFacItem->getWriteInhibitedFlag() )
    {
        context.m_pSecurityContext->setHasSecurityFileAssigned( true );
    }
}


//  asgResolveNameItems()
//
//  Resolves any appropriate name items to the current facility item.
void
FacilitiesManager::asgResolveNameItems
(
    const Context&                      context,
    const StandardFacilityItem* const   pFacilityItem
)
{
    const RunInfo::NAMEITEMS nameItems = context.m_pRunInfo->getNameItems();
    for ( RunInfo::CITNAMEITEMS itni = nameItems.begin(); itni != nameItems.end(); ++itni )
    {
        //  Only look at unresolved name items...
        const RunInfo::NameItem* pNameItem = itni->second;
        if ( pNameItem->m_FacilityItemIdentifier == FacilityItem::INVALID_IDENTIFIER )
        {
            //  Can this NameItem resolve to the indicated facility item?
            FileSpecification effectiveSpec;
            getEffectiveFileSpecification( context.m_pRunInfo, pNameItem->m_FileSpecification, &effectiveSpec );
            if ( compareFileSpecificationToFacilityItem( effectiveSpec, pFacilityItem ) )
            {
                RunInfo::NameItem* pDynamicNameItem = context.m_pRunInfo->getNameItem( pNameItem->m_Name );
                pDynamicNameItem->m_FacilityItemIdentifier = pFacilityItem->getIdentifier();
            }
        }
    }
}


