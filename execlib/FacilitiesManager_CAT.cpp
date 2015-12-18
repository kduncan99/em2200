//  Facilities class implementation
//
//  Subfunctions for @CAT



#include    "execlib.h"



//  TODO:REM need to handle removable semantics


//  private methods

//  catDisk()
//
//  Catalogs a disk file if possible
//
//  Parameters:
//      context:            context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catDisk
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    FileSpecification* const        pEffectiveSpec,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Check options
    UINT32 allowedOpts = OPTB_B | OPTB_G | OPTB_P | OPTB_R | OPTB_S | OPTB_V | OPTB_W | OPTB_Z;
    if ( !checkOptions( options, allowedOpts, context.m_pResult ) )
        return false;

    //  Does a fileset already exist?
    if ( pFileSetInfo )
        return catDiskCycle( context, options, additionalFields, pEffectiveSpec, pEquipmentType, pFileSetInfo );
    else
        return catDiskSetCycle( context, options, additionalFields, pEffectiveSpec, pEquipmentType );
}


//  catDiskCycle()
//
//  Catalogs a disk file if possible, where a fileset already exists.
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//      pFileSetInfo:           pointer to FileSetInfo from MFD
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catDiskCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    FileSpecification* const        pEffectiveSpec,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Make sure read and write keys were specified, if they exist on the fileset
    bool readDisabled = false;
    bool writeDisabled = false;
    if ( !checkKeys( context, *pEffectiveSpec, *pFileSetInfo, true, &readDisabled, &writeDisabled ) )
        return false;

    //  Check constraints on requested f-cycle
    UINT16 absoluteCycle = 0;
    if ( !checkCycleConstraints( context, *pEffectiveSpec, *pFileSetInfo, &absoluteCycle ) )
        return false;

    //  Validate additional subfields
    DiskSubfieldInfo dsInfo;
    dsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkDiskSubfields( context, additionalFields, 0, 0, &dsInfo, true ) )
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
                                                                   false );

    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_CAT::catDiskCycle", *pEffectiveSpec, mfdResult );
        return false;
    }

    return true;
}


//  catDiskSetCycle()
//
//  Catalogs a disk file if possible, where a fileset does not currently exist.
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catDiskSetCycle
(
    const Context&              context,
    const UINT32                options,
    const FieldList&            additionalFields,
    FileSpecification* const    pEffectiveSpec,
    const EquipmentType*        pEquipmentType
) const
{
    //  Check to ensure f-cycle is valid.
    //  We accept any valid absolute cycle, but only +1 for a relative cycle.
    //  If not specified, we default to absolute cycle 1.
    if ( pEffectiveSpec->m_AbsoluteCycleSpecified || pEffectiveSpec->m_RelativeCycleSpecified )
    {
        if ( !checkCycleConstraints( context, *pEffectiveSpec ) )
            return false;
    }

    UINT16 absoluteCycle = pEffectiveSpec->m_AbsoluteCycleSpecified ? pEffectiveSpec->m_Cycle : 1;

    //  Check additional subfields
    DiskSubfieldInfo dsInfo;
    dsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkDiskSubfields( context, additionalFields, 0, 0, &dsInfo, true ) )
        return false;

    //  Create file set
    DSADDR leadItem0Addr = 0;
    MFDManager::Result mfdResult = m_pMFDManager->createFileSet( context.m_pActivity,
                                                                 pEffectiveSpec->m_Qualifier,
                                                                 pEffectiveSpec->m_FileName,
                                                                 context.m_pRunInfo->getProjectId(),
                                                                 pEffectiveSpec->m_ReadKey,
                                                                 pEffectiveSpec->m_WriteKey,
                                                                 MFDManager::FILETYPE_MASS_STORAGE,
                                                                 (options & OPTB_G) == OPTB_G,
                                                                 &leadItem0Addr );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_CAT::catDiskSetCycle(a)", *pEffectiveSpec, mfdResult );
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

    mfdResult = m_pMFDManager->createFileCycle( context.m_pActivity,
                                                leadItem0Addr,
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
                                                false );

    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        //  Back out creation of fileset
        logMFDError( context, "FacilitiesManager_CAT::catDiskCycle(b)", *pEffectiveSpec, mfdResult );
        m_pMFDManager->dropFileSet( context.m_pActivity, leadItem0Addr );
        return false;
    }

    return true;
}


//  catTape()
//
//  Catalogs a tape file if possible.
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catTape
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    FileSpecification* const        pEffectiveSpec,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Check options
    UINT32 allowedOpts = OPTB_E | OPTB_G | OPTB_H | OPTB_J |  OPTB_L | OPTB_M | OPTB_O | OPTB_P | OPTB_R | OPTB_S | OPTB_V | OPTB_W | OPTB_Z;
    if ( !checkOptions( options, allowedOpts, context.m_pResult ) )
        return false;

    //  Does a fileset already exist?
    if ( pFileSetInfo )
        return catTapeCycle( context, options, additionalFields, pEffectiveSpec, pEquipmentType, pFileSetInfo );
    else
        return catTapeSetCycle( context, options, additionalFields, pEffectiveSpec, pEquipmentType );
}


//  catTapeCycle()
//
//  Catalogs a tape file if possible, where a fileset already exists.
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//      pFileSetInfo:           pointer to FileSetInfo from MFD
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catTapeCycle
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    FileSpecification* const        pEffectiveSpec,
    const EquipmentType*            pEquipmentType,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  Make sure read and write keys were specified, if they exist on the fileset
    bool readDisabled = false;
    bool writeDisabled = false;
    if ( !checkKeys( context, *pEffectiveSpec, *pFileSetInfo, true, &readDisabled, &writeDisabled ) )
        return false;

    //  Check constraints on requested f-cycle
    UINT16 absoluteCycle = 0;
    if ( !checkCycleConstraints( context, *pEffectiveSpec, *pFileSetInfo, &absoluteCycle ) )
        return false;

    //  Validate additional subfields
    TapeSubfieldInfo tsInfo;
    tsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkTapeSubfields( context, additionalFields, pFileSetInfo, 0, 0, &tsInfo ) )
        return false;

    //createFileCycle
    //TODO:TAPE

    return true;
}


//  catTapeSetCycle()
//
//  Catalogs a tape file if possible, where a fileset does not currently exist.
//
//  Parameters:
//      context:                context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pEquipmentType:         pointer to EquipmentType based on the additionalFields content
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catTapeSetCycle
(
    const Context&              context,
    const UINT32                options,
    const FieldList&            additionalFields,
    FileSpecification* const    pEffectiveSpec,
    const EquipmentType*        pEquipmentType
) const
{
    //  Check to ensure f-cycle is valid.
    //  We accept any valid absolute cycle, but only +1 for a relative cycle.
    //  If not specified, we default to absolute cycle 1.
    if ( !pEffectiveSpec->m_AbsoluteCycleSpecified && !pEffectiveSpec->m_RelativeCycleSpecified )
    {
        pEffectiveSpec->m_AbsoluteCycleSpecified = true;
        pEffectiveSpec->m_Cycle = 1;
    }
    else if ( !checkCycleConstraints( context, *pEffectiveSpec ) )
        return false;

    //  Check additional subfields
    TapeSubfieldInfo tsInfo;
    tsInfo.m_pEquipmentType = pEquipmentType;
    if ( !checkTapeSubfields( context, additionalFields, 0, 0, 0, &tsInfo ) )
        return false;

    //  Create file set
    DSADDR leadItem0Addr = 0;
    MFDManager::Result mfdResult = m_pMFDManager->createFileSet( context.m_pActivity,
                                                                 pEffectiveSpec->m_Qualifier,
                                                                 pEffectiveSpec->m_FileName,
                                                                 context.m_pRunInfo->getProjectId(),
                                                                 pEffectiveSpec->m_ReadKey,
                                                                 pEffectiveSpec->m_WriteKey,
                                                                 MFDManager::FILETYPE_TAPE,
                                                                 (options & OPTB_G) == OPTB_G,
                                                                 &leadItem0Addr );
    if ( !checkMFDResult( mfdResult, context.m_pResult ) )
    {
        logMFDError( context, "FacilitiesManager_CAT::catTapeSetCycle", *pEffectiveSpec, mfdResult );
        return false;
    }

    //createFileCycle
    //TODO:TAPE

    return true;
}


//  catUnknown()
//
//  Catalogs a file if possible.  The caller did not specify an equipment type.
//  Thus, if a file set is already cataloged for the qual*file combination, we use that fileset's general
//  type (TAPE or WORD-MASS-STORAGE) to select the system-configured default equipment type.
//  Otherwise, we assume (sector-addressable) mass storage.
//
//  Parameters:
//      context:            context under which we operate
//      options:                catalog options
//      additionalFields:       additional fields from the cat image
//      pEffectiveSpec:         pointer to effective file specification
//      pFileSetInfo:           pointer to FileSetInfo from MFD if a fileset already exists
//
//  Returns:
//      true if successful, else false
bool
FacilitiesManager::catUnknown
(
    const Context&                  context,
    const UINT32                    options,
    const FieldList&                additionalFields,
    FileSpecification* const        pEffectiveSpec,
    const MFDManager::FileSetInfo*  pFileSetInfo
) const
{
    //  If a fileset exists and it's tape, use the default tape equipment type to catalog a new tape cycle.
    if ( pFileSetInfo && pFileSetInfo->m_FileType == MFDManager::FILETYPE_TAPE )
        return catTape( context, options, additionalFields, pEffectiveSpec, m_pDefaultEquipTape, pFileSetInfo );

    //  Otherwise, fileset or not, we're going to use the default sector-addressable equipment type
    //  to catalog a new disk cycle.
    return catDisk( context, options, additionalFields, pEffectiveSpec, m_pDefaultEquipMassStorage, pFileSetInfo );
}


