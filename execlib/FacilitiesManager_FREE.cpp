//  Subset of FacilitiesManager implementation
//
//  This file includes functions dealing with the release of facilities items



#include    "execlib.h"



//  private, protected methods

//  freeInternalName()
//
//  User has specified either A or B option (and possibly others).
//  A option releases only the internal file name.
//  B option does this, and if there are no more internal file names, releases the file as well.
//  Some of the other interesting options are allowed with B, for the case where the file is released.
bool
FacilitiesManager::freeInternalName
(
    const Context&      context,
    const UINT32        options,
    const SuperString&  internalName
) const
{
    //  Not allowed with A option: B, D, I, R, S
    //  Not allowed with B option: A, R, X
    bool aOpt = (options & OPTB_A) != 0;
    bool bOpt = (options & OPTB_B) != 0;
    if ( aOpt )
    {
        if ( !checkOptionConflicts( context, options, OPTB_A | OPTB_B | OPTB_D | OPTB_I | OPTB_R | OPTB_S ) )
            return false;
    }
    if ( bOpt )
    {
        if ( !checkOptionConflicts( context, options, OPTB_A | OPTB_B | OPTB_R | OPTB_X ) )
            return false;
    }

    //  First of all, find the NameItem.
    RunInfo::NameItem* pNameItem = context.m_pRunInfo->getNameItem( internalName );
    if ( !pNameItem )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_NAME_NOT_KNOWN_TO_THIS_RUN );
        context.m_pResult->m_StatusBitMask.logicalOr( 0500000000000ll );
        return false;
    }

    //  Are we b-option and is the file assigned to this run?
    if ( bOpt && ( pNameItem->m_FacilityItemIdentifier != FacilityItem::INVALID_IDENTIFIER ) )
    {
        //  Count number of name items which reference this file.
        COUNT refs = 0;
        const RunInfo::NAMEITEMS& nameItems = context.m_pRunInfo->getNameItems();
        for ( RunInfo::CITNAMEITEMS itni = nameItems.begin(); itni != nameItems.end(); ++itni )
        {
            if ( itni->second->m_FacilityItemIdentifier == pNameItem->m_FacilityItemIdentifier )
                ++refs;
        }

        //  If the count is only one (that is, this is the only name item) we need to release the file.
        if ( refs == 1 )
        {
            FacilityItem* pfi = context.m_pRunInfo->getFacilityItem( pNameItem->m_FacilityItemIdentifier );
            if ( pfi == 0 )
            {
                std::stringstream strm;
                strm << "FacilitiesManager_FREE::freeInternalName() cannot find FacItem id=" << pNameItem->m_FacilityItemIdentifier;
                SystemLog::getInstance()->write( strm.str() );
                m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
                return false;
            }

            freeReleaseFacilityItem( context, options, pfi );
        }
    }

    //  Release this name item and we're done.
    context.m_pRunInfo->discardNameItem( internalName );
    return true;
}


//  freeReleaseDiskFile()
//
//  Releases a disk file from the RunInfo object in the context object.
//  Handles release unused initial reserve, notifying the MFD to update the main/lead items,
//  and updating the FacItems container in the RunInfo object.
//  Also demotes the privilege if this is SYS$*DLOC$.
//  If this is a +1 file cycle, MFDManager will automatically take care of permanently cataloging the file.
//
//  Parameters:
//      context:        The usual context information (we might update the security context)
//      pFacilityItem:  pointer to facility item describing the disk file to be released
//      deleteFlag:     true to force deletion of the file cycle.
//
//  Returns:
//      true generallly, false is likely to indicate an impending EXEC stop
bool
FacilitiesManager::freeReleaseDiskFile
(
    const Context&              context,
    DiskFacilityItem* const     pFacilityItem,
    const bool                  deleteFlag
) const
{
    //  If the file is temporary, we need to notify MFD to release allocated tracks.
    if ( pFacilityItem->getTemporaryFileFlag() )
    {
        MFDManager::Result mfdResult = m_pMFDManager->releaseFileTracks( context.m_pActivity,
                                                                         pFacilityItem,
                                                                         0,
                                                                         pFacilityItem->getHighestTrackWritten() );
        if ( mfdResult.m_Status == MFDManager::MFDST_IO_ERROR )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MFD_IO_ERROR_DURING_FREE );
        }
        else if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
        {
            logMFDError( context, "FacilitiesManager_FREE::freeReleaseDiskFile(a)", pFacilityItem, mfdResult );
            return false;
        }
    }

    //  Not temporary file...
    else
    {
        //  Possibly release unused initial reserve
        if ( m_ReleaseUnusedReserve )
            freeReleaseDiskUnusedInitialReserve( context, pFacilityItem );

        //  Tell the MFD to update the file items such that MFD no longer accounts the file as assigned
        MFDManager::Result mfdResult = m_pMFDManager->releaseFileCycle( context.m_pActivity,
                                                                        pFacilityItem->getMainItem0Addr() );
        if ( mfdResult.m_Status == MFDManager::MFDST_IO_ERROR )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MFD_IO_ERROR_DURING_FREE );
        }
        else if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
        {
            logMFDError( context, "FacilitiesManager_FREE::freeReleaseDiskFile(b)", pFacilityItem, mfdResult );
            return false;
        }

        //  Is it DLOC$?
        if ( !pFacilityItem->getTemporaryFileFlag()
            && (pFacilityItem->getQualifier().compareNoCase( "SYS$" ) == 0)
            && (pFacilityItem->getFileName().compareNoCase( "DLOC$" ) == 0) )
        {
            context.m_pSecurityContext->setHasSecurityFileAssigned( false );
        }

        //  Does it need to be deleted?
        if ( deleteFlag )
        {
            //  Tell MFD to delete the file
            mfdResult = m_pMFDManager->dropFileCycle( context.m_pActivity, pFacilityItem->getMainItem0Addr(), true );
            if ( mfdResult.m_Status == MFDManager::MFDST_FILE_ON_PRINT_QUEUE )
            {
                //  There is apparently no status bit we can use for this, so just push the status code.
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_ON_PRINT_QUEUE );
                logMFDError( context, "FacilitiesManager_FREE::freeReleaseDiskFile(c)", pFacilityItem, mfdResult );
            }
            else if ( mfdResult.m_Status == MFDManager::MFDST_IO_ERROR )
            {
                logMFDError( context, "FacilitiesManager_FREE::freeReleaseDiskFile(c)", pFacilityItem, mfdResult );
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MFD_IO_ERROR_DURING_FREE );
                return false;
            }
            else if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
            {
                logMFDError( context, "FacilitiesManager_FREE::freeReleaseDiskFile(c)", pFacilityItem, mfdResult );
                return false;
            }
        }
    }

    //  Remove from RunInfo
    context.m_pRunInfo->removeFacilityItem( pFacilityItem->getIdentifier() );
    return true;
}


//  freeReleaseDiskUnusedInitialReserve()
//
//  Releases unused initial reserve from the referenced disk file
//  Don't call here if we're not configured to do this.
//
//  Returns true if
//      We successfully released unused initial reserve or
//      There was no need to release anything
//  Returns false if
//      We tried to release unused initial reserve and failed (Implies EXEC has been signalled to stop)
bool
FacilitiesManager::freeReleaseDiskUnusedInitialReserve
(
    const Context&          context,
    DiskFacilityItem* const pFacilityItem
) const
{
    bool result = true;

    //  Is there anything to be released?  Don't bother if it's temporary or write-protected
    if ( !pFacilityItem->getTemporaryFileFlag() && !pFacilityItem->getWriteInhibitedFlag() )
    {
        TRACK_COUNT highestTrackAssigned = pFacilityItem->getHighestGranuleAssigned();
        if ( pFacilityItem->getGranularity() == MSGRAN_POSITION )
            highestTrackAssigned *= 64;
        if ( highestTrackAssigned > pFacilityItem->getHighestTrackWritten() )
        {
            //  Note it in the log TODO:DEBUG (only for development debugging - remove this later)
            std::stringstream logStrm;
            logStrm << "FacilitiesManager_FREE::freeReleaseDiskUnusedInitialReserve() RUNID="
                << context.m_pRunInfo->getActualRunId()
                << " file=" << pFacilityItem->getQualifier() << "*" << pFacilityItem->getFileName()
                << " highestTrkAsg=" << highestTrackAssigned
                << " highestTrkWrt=" << pFacilityItem->getHighestTrackWritten();
            SystemLog::write( logStrm.str() );
        }

        if ( highestTrackAssigned > pFacilityItem->getHighestTrackWritten() )
        {
            TRACK_ID releaseId = pFacilityItem->getHighestTrackWritten() + 1;
            TRACK_COUNT releaseCount = highestTrackAssigned - pFacilityItem->getHighestTrackWritten();
            MFDManager::Result mfdResult = m_pMFDManager->releaseFileTracks( context.m_pActivity,
                                                                             pFacilityItem,
                                                                             releaseId,
                                                                             releaseCount );
            if ( mfdResult.m_Status == MFDManager::MFDST_IO_ERROR )
            {
                logMFDError( context, "FacilitiesManager::freeReleaseDiskUnusedInitialReserve()", pFacilityItem, mfdResult );
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MFD_IO_ERROR_DURING_FREE );
            }
            else if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
            {
                logMFDError( context, "FacilitiesManager::freeReleaseDiskUnusedInitialReserve()", pFacilityItem, mfdResult );
                return false;
            }
        }
    }

    return result;
}


//  freeReleaseFacilityItem()
//
//  Releases the facility associated with the given FacItem.
//  May do other things, depending upon the given options (from the @FREE statement),
//  the assign options on the FacItem, and the type of the FacItem.
//
//  One thing of note: we delay checking options until we get here, since we cannot know
//  for sure beforehand which options are valid.
bool
FacilitiesManager::freeReleaseFacilityItem
(
    const Context&          context,
    const UINT32            options,
    FacilityItem* const     pFacilityItem
) const
{
    //  Code for handling mass-storage files
    DiskFacilityItem* pdfi = dynamic_cast<DiskFacilityItem*>( pFacilityItem );
    if ( pdfi )
    {
        //  Check options
        UINT32 allowed = OPTB_A | OPTB_B | OPTB_R | OPTB_X;
        if ( !pdfi->getTemporaryFileFlag() )
            allowed |= OPTB_D;
        if ( pdfi->getCatalogOnAnyTermFlag() || pdfi->getCatalogOnNormalTermFlag() )
            allowed |= OPTB_I;
        if ( !checkOptions( options, allowed, context.m_pResult ) )
            return false;

        //  Need to delete the file after release?
        bool deleteFlag = pdfi->getDeleteOnAnyTermFlag() || pdfi->getDeleteOnNormalTermFlag();
        if ( options & (OPTB_D | OPTB_I) )
            deleteFlag = true;

        return freeReleaseDiskFile( context, pdfi, deleteFlag );
    }

    //  Code for handling tape files (as well as the underlying assigned device, if any)
    TapeFacilityItem* ptfi = dynamic_cast<TapeFacilityItem*>( pFacilityItem );
    if ( ptfi )
    {
        //  Check options
        UINT32 allowed = OPTB_A | OPTB_B | OPTB_R | OPTB_S;
        if ( !ptfi->getTemporaryFileFlag() )
            allowed |= OPTB_D;
        if ( ptfi->getCatalogOnAnyTermFlag() || ptfi->getCatalogOnNormalTermFlag() )
            allowed |= OPTB_I;
        if ( !checkOptions( options, allowed, context.m_pResult ) )
            return false;

        //  Need to delete the file after release?
        bool deleteFlag = ptfi->getDeleteOnAnyTermFlag() || ptfi->getDeleteOnNormalTermFlag();
        if ( options & (OPTB_D | OPTB_I) )
            deleteFlag = true;

        return freeReleaseTapeFile( context, ptfi, deleteFlag );
    }

    //  Code for handling absolute assigned devices (this will always be ASG,T)
    const NonStandardFacilityItem* pnsfi = dynamic_cast<const NonStandardFacilityItem*>( pFacilityItem );
    if ( pnsfi )
    {
        UINT32 allowed = OPTB_A | OPTB_B | OPTB_R | OPTB_S;
        if ( !checkOptions( options, allowed, context.m_pResult ) )
            return false;

        //TODO:ABSASG
        return true;
    }

    std::string logMsg = "FacilitiesManager_FREE::freeReleaseFacilityItem():Undeterminable fac item type for ";
    logMsg += pFacilityItem->getQualifier() + "*" + pFacilityItem->getFileName();
    SystemLog::write( logMsg );
    m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );

    return false;
}


//  freeReleaseTapeFile()
//
//  Releases a tape file from the RunInfo object in the context object.
//  Handles release unused initial reserve, notifying the MFD to update the main/lead items,
//  and updating the FacItems container in the RunInfo object.
//  If this is a +1 file cycle, MFDManager will automatically take care of permanently cataloging the file.
//
//  Parameters:
//      context:        The usual context information
//      pFacilityItem:  pointer to facility item describing the tape file to be released
//      deleteFlag:     true to force deletion of the file cycle.
//
//  Returns:
//      true generallly, false is likely to indicate an impending EXEC stop
bool
FacilitiesManager::freeReleaseTapeFile
(
    const Context&                  context,
    TapeFacilityItem* const         pFacilityItem,
    const bool                      deleteFlag
) const
{
    return true;//TODO:TAPE
}



//  private, protected static methods

//  freeReleaseNameItems()
//
//  Release all NameItem objects from the given RunInfo object,
//  which refer to the given facility item.
void
FacilitiesManager::freeReleaseNameItems
(
    const Context&              context,
    FacilityItem::IDENTIFIER    facilityItemIdentifier
)
{
    //  Be careful when iterating over the name list while we're editing it.
    const RunInfo::NAMEITEMS& nameItems = context.m_pRunInfo->getNameItems();
    RunInfo::CITNAMEITEMS itni = nameItems.begin();
    while ( itni != nameItems.end() )
    {
        RunInfo::CITNAMEITEMS itnext = itni;
        ++itnext;

        const RunInfo::NameItem* pNameItem = itni->second;
        if ( pNameItem->m_FacilityItemIdentifier == facilityItemIdentifier )
            context.m_pRunInfo->discardNameItem( pNameItem->m_Name );

        itni = itnext;
    }
}


//  freeUnresolveNameItems()
//
//  Un-resolves all NameItem objects from the given RunInfo object,
//  which refer to the given facility item.
void
FacilitiesManager::freeUnresolveNameItems
(
    const Context&              context,
    FacilityItem::IDENTIFIER    facilityItemIdentifier
)
{
    const RunInfo::NAMEITEMS& nameItems = context.m_pRunInfo->getNameItems();
    for ( RunInfo::CITNAMEITEMS itni = nameItems.begin(); itni != nameItems.end(); ++itni )
    {
        RunInfo::NameItem* pNameItem = itni->second;
        if ( pNameItem->m_FacilityItemIdentifier == facilityItemIdentifier )
            pNameItem->m_FacilityItemIdentifier = FacilityItem::INVALID_IDENTIFIER;
    }
}

