//  FileAllocationTable implementation



#include    "execlib.h"



//  private methods



//  private static methods

//  compareDADContent
//
//  Compares the allocation information of two DAD entries to see if they are identical.
bool
FileAllocationTable::compareDADContent
(
    const DeviceAreaDescriptor* const   pDAD1,
    const DeviceAreaDescriptor* const   pDAD2
)
{
    if ( pDAD1->getFirstFileRelativeWord() != pDAD2->getFirstFileRelativeWord() )
        return false;
    if ( pDAD1->getLastFileRelativeWord() != pDAD2->getLastFileRelativeWord() )
        return false;
    for ( INDEX ex = 0 ; ex < 8 ; ++ex )
    {
        if ( pDAD1->getEntryDiskAddress( ex ) != pDAD2->getEntryDiskAddress( ex ) )
            return false;
        if ( pDAD1->getEntryLDATIndex( ex ) != pDAD2->getEntryLDATIndex( ex ) )
            return false;
        if ( pDAD1->getEntryWordLength( ex ) != pDAD2->getEntryWordLength( ex ) )
            return false;
        //  Don't need to check removable flag, this comparison should only be used
        //  on enties for one particular file - so it should always match.
        if ( pDAD1->isLastEntry( ex ) != pDAD2->isLastEntry( ex ) )
            return false;
        if ( pDAD1->isLastEntry( ex ) )
            break;
    }

    return true;
}



//  constructors, destructors



//  public methods

//  allocated()
//
//  Marks a range of tracks as allocated.
//  It would be an error in software for us to be asked to mark a track which is already
//  allocated, so in that case we return false and calling code should do something drastic (crash?)
bool
FileAllocationTable::allocated
(
    const TRACK_ID      fileRelativeFirstTrackId,
    const TRACK_COUNT   trackCount,
    const LDATINDEX     ldatIndex,
    const TRACK_ID      deviceRelativeFirstTrackId
)
{
    lock();

    //  If the map is empty, this is trivial.
    if ( m_Entries.empty() )
    {
        m_Entries[fileRelativeFirstTrackId] = FileAllocationEntry( trackCount, ldatIndex, deviceRelativeFirstTrackId );
        m_IsUpdated = true;

        unlock();
        return true;
    }

    //  Find first entry beyond the starting track (if such exists).
    //  Also find the entry which immediately precedes the requested allocation (if such exists).
    ITFAENTRIES itUpperBound = m_Entries.upper_bound( fileRelativeFirstTrackId );
    ITFAENTRIES itLowerBound = m_Entries.end();
    if ( itUpperBound != m_Entries.begin() )
    {
        itLowerBound = itUpperBound;
        --itLowerBound;
    }

    //  If there's a lower bound, check for overlap with requested region, and for adjacency.
    //  Note that adjacency requires not only file-relative adjacent regions, but also
    //  physical device-relative adjacent regions.
    bool adjacentLower = false;
    if ( itLowerBound != m_Entries.end() )
    {
        TRACK_ID prevRegionLimit = itLowerBound->first + itLowerBound->second.m_TrackCount;
        if ( prevRegionLimit > fileRelativeFirstTrackId )
            return false;
        if ( (prevRegionLimit == fileRelativeFirstTrackId)
                && ( itLowerBound->second.m_LDATIndex == ldatIndex )
                && ( itLowerBound->second.m_DeviceTrackId + itLowerBound->second.m_TrackCount == deviceRelativeFirstTrackId ) )
            adjacentLower = true;
    }

    //  If there's an upper bound, check for overlap and adjacency
    bool adjacentUpper = false;
    if ( itUpperBound != m_Entries.end() )
    {
        TRACK_ID reqRegionLimit = fileRelativeFirstTrackId + trackCount;
        if ( reqRegionLimit > itUpperBound->first )
            return false;
        if ( reqRegionLimit == itUpperBound->first
            && ( ldatIndex == itUpperBound->second.m_LDATIndex )
            && ( deviceRelativeFirstTrackId + trackCount == itUpperBound->second.m_DeviceTrackId ) )
            adjacentUpper = true;
    }

    //  If we are adjacent at both the top and bottom, then the caller is
    //  completely fill in a hole.  Just extend the lower entry to encompass
    //  the newly-allocated region as well as the upper region, and get rid
    //  of the upper region entry.
    if ( adjacentLower && adjacentUpper )
    {
        itLowerBound->second.m_TrackCount += trackCount + itUpperBound->second.m_TrackCount;
        m_Entries.erase( itUpperBound );
        m_IsUpdated = true;

        unlock();
        return true;
    }

    //  If we're adjacent to the lower region, just extend it to incorporate the included region.
    if ( adjacentLower )
    {
        itLowerBound->second.m_TrackCount += trackCount;
        m_IsUpdated = true;

        unlock();
        return true;
    }

    //  If we're adjacent to the upper region, extend it.  This is a bit trickier...
    //  We have to map a new key to the modified values, and lose the original mapping.
    if ( adjacentUpper )
    {
        m_Entries[fileRelativeFirstTrackId] = FileAllocationEntry( trackCount + itUpperBound->second.m_TrackCount,
                                                                   ldatIndex,
                                                                   deviceRelativeFirstTrackId );
        m_Entries.erase( itUpperBound );
        m_IsUpdated = true;

        unlock();
        return true;
    }

    //  We're not adjacent at all - just create a new entry
    m_Entries[fileRelativeFirstTrackId] = FileAllocationEntry( trackCount, ldatIndex, deviceRelativeFirstTrackId );
    m_IsUpdated = true;

    unlock();
    return true;
}


//  buildFileAllocationEntries()
//
//  Caller has loaded the DAD tables from the MFD, and now wants to create the FAE's
//  based on the contents of the DAD tables.  Note that the caller may just as easily
//  load the FAE's from the DAD tables in the MFD, after which he should call synchronizeDADTables().
void
FileAllocationTable::buildFileAllocationEntries()
{
    lock();

    m_Entries.clear();
    for ( ITDADTABLES itdt = m_DADTables.begin(); itdt != m_DADTables.end(); ++itdt )
    {
        WORD_ID entryWordId = (*itdt)->getFirstFileRelativeWord();
        for ( INDEX ex = 0; ex < 8; ++ex )
        {
            if ( (*itdt)->getEntryLDATIndex( ex ) != 0400000 )
            {
                TRACK_ID entryTrackId = entryWordId / 1792;
                TRACK_COUNT entryTrackCount = (*itdt)->getEntryWordLength( ex ) / 1792;
                TRACK_ID entryDiskTrackId = (*itdt)->getEntryDiskAddress( ex ) / 1792;
                allocated( entryTrackId, entryTrackCount, (*itdt)->getEntryLDATIndex( ex ), entryDiskTrackId );
            }

            entryWordId += (*itdt)->getEntryWordLength( ex );
            if ( (*itdt)->isLastEntry( ex ) )
                break;
        }
    }

    m_IsUpdated = false;
    unlock();
}


//  clear()
//
//  Corresponds to a request to release all the tracks of the file.
void
FileAllocationTable::clear()
{
    lock();

    for ( ITDADTABLES itdt = m_DADTables.begin(); itdt != m_DADTables.end(); ++itdt )
        delete (*itdt);
    m_DADTables.clear();
    m_Entries.clear();

    unlock();
}


//  convertTrackId()
//
//  Converts a file-relative track ID to the ldat index of the containing device
//  and the corresonding device-relative track ID.
//  Returns true generally; false if the file-relative track ID is not allocated
//  (including the case where it is out of range).
bool
FileAllocationTable::convertTrackId
(
    const TRACK_ID      fileRelativeTrackId,
    LDATINDEX* const    pLDATIndex,
    TRACK_ID* const     pDeviceRelativeTrackId
) const
{
    lock();

    for ( CITFAENTRIES itfae = m_Entries.begin(); itfae != m_Entries.end(); ++itfae )
    {
        if ( itfae->first > fileRelativeTrackId )
            break;

        TRACK_COUNT offset = fileRelativeTrackId - itfae->first;
        if ( (fileRelativeTrackId >= itfae->first) && (offset < itfae->second.m_TrackCount) )
        {
            *pLDATIndex = itfae->second.m_LDATIndex;
            *pDeviceRelativeTrackId = itfae->second.m_DeviceTrackId + offset;
            unlock();
            return true;
        }
    }

    unlock();
    return false;
}


//  dump()
//
//  For debugging
void
FileAllocationTable::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const std::string&  fileIdentifier
) const
{
    lock();

    stream << prefix << "FileAllocationTable for " << fileIdentifier
            << "  MainItem0=0" << std::oct << std::setw(12) << std::setfill('0') << m_MainItem0Addr
            << (m_IsRemovable ? " *REMOVABLE*" : "")
            << (m_IsUpdated ? " *UPDATED* " : "")
            << std::endl;

    stream << prefix << "  FileAllocationEntries -----------------" << std::endl;
    for ( CITFAENTRIES itfae = m_Entries.begin(); itfae != m_Entries.end(); ++itfae )
    {
        stream << prefix << "    LogTrk:0" << std::oct << itfae->first
            << " TrkCnt:0" << std::oct << itfae->second.m_TrackCount
            << "  LDAT:0" << std::oct << itfae->second.m_LDATIndex
            << "  DevTrk:0" << std::oct << itfae->second.m_DeviceTrackId << std::endl;
    }

    stream << prefix << "  DAD Tables ----------------------------" << std::endl;
    for ( CITDADTABLES itdt = m_DADTables.begin(); itdt != m_DADTables.end(); ++itdt )
    {
        stream << prefix << "    DAD DSADDR:               "
                << std::oct << std::setw(12) << std::setfill('0') << (*itdt)->getDSAddress()
                << std::endl;
        stream << prefix << "    FirstFileRelWordAddr:     "
                << std::oct << std::setw(12) << std::setfill('0') << (*itdt)->getFirstFileRelativeWord()
                << std::endl;
        stream << prefix << "    LastFileRelWordAddrPlus1: "
                << std::oct << std::setw(12) << std::setfill('0') << (*itdt)->getLastFileRelativeWord()
                << std::endl;

        for ( INDEX ex = 0; ex < 8; ++ex )
        {
            stream << prefix << "      [" << std::oct << std::setw(3) << std::setfill('0') << "]";
            if ( (*itdt)->getEntryLDATIndex( ex ) != 0400000 )
            {
                stream << "  LDAT:" << std::oct << std::setw(6) << std::setfill('0') << (*itdt)->getEntryLDATIndex( ex );
                stream << "  DRWA:" << std::oct << std::setw(12) << std::setfill('0') << (*itdt)->getEntryDiskAddress( ex );
            }
            else
                stream << "  *UNALLOC*";
            stream << "  Words:" << std::oct << std::setw(12) << std::setfill('0') << (*itdt)->getEntryWordLength( ex );
            stream << ((*itdt)->isLastEntry( ex ) ? "  *LAST*" : "") << ((*itdt)->isRemovableEntry( ex ) ? "  *REM*" : "");
            stream << std::endl;
            if ( (*itdt)->isLastEntry( ex ) )
                break;
        }
    }

    unlock();
}


//  getAllocatedTrackCount()
//
//  Not sure if we'll use this, but here it is.  Counts number of allocated tracks.
TRACK_COUNT
FileAllocationTable::getAllocatedTrackCount() const
{
    TRACK_COUNT count = 0;
    lock();

    for ( CITFAENTRIES itfae = m_Entries.begin(); itfae != m_Entries.end(); ++itfae )
        count += itfae->second.m_TrackCount;

    unlock();
    return count;
}


//  getFileAllocationEntries()
//
//  Populates the given container with FAE's describing the region indicated by the given
//  trackId and trackCount.  For small trackCount values, we may return only one entry
//  which represents just a subset of some existing entry.  For larger trackCount values,
//  we might return a subset at the front and the back, and multiple full entries in the middle.
//
//  When directed to allocate space for a file, the caller will invoke this routine, then
//  allocate disk space for each of the indicated entries which are not allocated, ignoring
//  the areas within the allocation request for which space is already allocated.
//
//  When directed to release space for a file, the caller will again invoke this routine,
//  then release disk space for each of the indicated entries which *are* allocated, ignoring
//  the areas within the release request for which space is not allocated.
//
//  For *THIS* case, we introduce the concept of an unallocated FAE - it is the same FAE
//  used for tracking allocations; but we indicate that it represents a 'hole' in the
//  allocations, by setting LDAT index (and device track ID) to zero.
void
FileAllocationTable::getFileAllocationEntries
(
    const TRACK_ID      firstTrackId,   //  file-relative track ID
    const TRACK_COUNT   trackCount,     //  number of tracks caller is interested in
    FAENTRIES* const    pEntries        //  caller's container which we populate
) const
{
    pEntries->clear();

    //  Empty request?  Shouldn't happen, but we'll protect ourselves just in case.
    if ( trackCount == 0 )
        return;

    lock();

    //  Set up working values which we can loop over and modify.
    TRACK_ID workingTrackId = firstTrackId;
    TRACK_COUNT workingTrackCount = trackCount;

    //  Find iterators referring to the FAE which contains the working track ID
    //  and the FAE which follows that FAE.  If working track ID is in an unallocated
    //  region, the containing iterator will be .end().  It may be the case that the
    //  following iterator will be .end() - indicating that we are at, or near,
    //  the end of the allocations.  This may result in the last entry in the container,
    //  being an unallocated entry.
    CITFAENTRIES itContaining = m_Entries.end();
    CITFAENTRIES itFollowing = m_Entries.upper_bound( firstTrackId );
    if ( itFollowing != m_Entries.begin() )
    {
        itContaining = itFollowing;
        --itContaining;
    }

    while ( workingTrackCount > 0 )
    {
        //  Is the next entry to be created, an unallocated entry?
        if ( itContaining == m_Entries.end()
            || ( workingTrackId >= itContaining->first + itContaining->second.m_TrackCount ) )
        {
            //  Yes - find the size of this entry - if we are at the end of m_Entries,
            //  use all the remaining workingTrackCount; otherwise, use the beginning of
            //  the following entry to determine the size of *this* entry.
            TRACK_COUNT entrySize = workingTrackCount;
            if ( itFollowing != m_Entries.end() )
            {
                TRACK_COUNT margin = itFollowing->first - workingTrackId;
                if ( margin < entrySize )
                    entrySize = margin;
            }

            //  Create the entry, then update working values
            (*pEntries)[workingTrackId] = FileAllocationEntry( entrySize, 0, 0 );
            workingTrackId += entrySize;
            workingTrackCount -= entrySize;

            //  Finally, update iterators as appropriate
            itContaining = itFollowing;
            if ( itFollowing != m_Entries.end() )
                ++itFollowing;
        }
        else
        {
            //  This is a normal allocation entry.  Use as much of the containing entry as possible.
            TRACK_COUNT entrySize = itContaining->first + itContaining->second.m_TrackCount - workingTrackId;
            if ( entrySize > workingTrackCount )
                entrySize = workingTrackCount;
            TRACK_COUNT trackOffset = workingTrackId - itContaining->first;
            (*pEntries)[workingTrackId] = FileAllocationEntry( entrySize,
                                                               itContaining->second.m_LDATIndex,
                                                               itContaining->second.m_DeviceTrackId + trackOffset );
            workingTrackId += entrySize;
            workingTrackCount -= entrySize;

            //  If the next entry abuts this entry (which could be the case for logically contiguous
            //  but not physically contiguous allocations), adjust the iterators.  If the next entry
            //  is *not* logically contiguous, leave the iterators alone - next loop iteration will
            //  take the non-allocation branch (as it should).
            if ( itFollowing != m_Entries.end()
                && ( itContaining->first + itContaining->second.m_TrackCount == itFollowing->first ) )
            {
                itContaining = itFollowing;
                ++itFollowing;
            }
        }
    }

    unlock();
}


//  getHighestTrackAssigned()
TRACK_ID
FileAllocationTable::getHighestTrackAssigned() const
{
    TRACK_ID trackId = 0;
    lock();

    if ( !m_Entries.empty() )
    {
        CITFAENTRIES itfae = m_Entries.end();
        --itfae;
        trackId = itfae->first + itfae->second.m_TrackCount - 1;
    }

    unlock();
    return trackId;
}


//  released()
//
//  Marks a region of allocated tracks as released.
//  Note that this might span multiple entries, perhaps a subset of the beginning and ending entry.
//
//  Returns true generally; false if one or more of the tracks in the requested region
//  were not allocated to begin with.
//  A false return indicates a bad software error, so caller should do something drastic in this case.
//  Note that we might not return false until we've already done some work, so "drastic" should mean "crash"
bool
FileAllocationTable::released
(
    const TRACK_ID      fileRelativeFirstTrackId,
    const TRACK_COUNT   trackCount
)
{
    lock();

    TRACK_ID fileTrackId = fileRelativeFirstTrackId;
    TRACK_COUNT tracksLeft = trackCount;
    while ( tracksLeft )
    {
        //  Find the entry containing the fileTrackId.
        bool found = false;
        ITFAENTRIES itfae = m_Entries.begin();
        while ( itfae != m_Entries.end() )
        {
            if ( itfae->first > fileTrackId )
                break;
            if ( itfae->first + itfae->second.m_TrackCount > fileTrackId )
            {
                found = true;
                break;
            }
            ++itfae;
        }

        if ( !found )
        {
            unlock();
            return false;
        }

        //  Okay - fileTrackId is within the entry referenced by itfae.
        //  Is the entry completely overlapped by the requested region?  If so, wack the whole entry.
        if ( (itfae->first == fileTrackId) && (itfae->second.m_TrackCount <= tracksLeft) )
        {
            tracksLeft -= itfae->second.m_TrackCount;
            fileTrackId += itfae->second.m_TrackCount;
            m_Entries.erase(itfae);
            continue;
        }

        //  From here downward, we only have to worry about a subset of the containing entry.
        //  Which means more work for us, possibly a lot more.

        //  Are we overlapping through to the top of the entry?  If so, we can resize the entry.
        if ( (fileTrackId + tracksLeft) >= (itfae->first + itfae->second.m_TrackCount) )
        {
            TRACK_COUNT overlappingTracks = itfae->first + itfae->second.m_TrackCount - fileTrackId;
            itfae->second.m_TrackCount -= overlappingTracks;
            tracksLeft -= overlappingTracks;
            fileTrackId += overlappingTracks;
            continue;
        }

        //  Are we adjacent to the bottom of the entry?  If so, we need to 'move' the entry...
        if ( itfae->first == fileTrackId )
        {
            m_Entries[fileTrackId + tracksLeft] = FileAllocationEntry( itfae->second.m_TrackCount - tracksLeft,
                                                                       itfae->second.m_LDATIndex,
                                                                       itfae->second.m_DeviceTrackId + tracksLeft );
            m_Entries.erase( itfae );
            fileTrackId += tracksLeft;
            tracksLeft = 0;
            continue;
        }

        //  <sigh> - we're removing a piece from the middle of the entry.
        //  This means we need to create a new entry for the back side, and modify the entry for the front side.
        TRACK_COUNT oldEntryUpdatedTrackCount = fileTrackId - itfae->first;
        TRACK_ID newEntryFirstTrackId = fileTrackId + tracksLeft;
        LDATINDEX newEntryLDATIndex = itfae->second.m_LDATIndex;
        TRACK_COUNT newEntryTrackCount = itfae->second.m_TrackCount - oldEntryUpdatedTrackCount - tracksLeft;
        TRACK_ID newEntryDeviceTrackId = itfae->second.m_DeviceTrackId + oldEntryUpdatedTrackCount + tracksLeft;

        itfae->second.m_TrackCount = oldEntryUpdatedTrackCount;
        m_Entries[newEntryFirstTrackId] = FileAllocationEntry( newEntryTrackCount, newEntryLDATIndex, newEntryDeviceTrackId );

        fileTrackId += tracksLeft;
        tracksLeft = 0;
    }

    m_IsUpdated = true;
    unlock();
    return true;
}


//  synchronizeDADTables()
//
//  After a client updates the configuration via allocated() or released(), the client
//  should call here to update our cached DAD tables, and should eventually rewrite any
//  updated DAD tables out to the MFD.
//
//  We iterate over the FileAllocationEntry structs.  They are kept coalesced such that
//  no two subsequent entries are logically contiguous (i.e., file-relative track
//  addresses are upwardly-contiguous, LDAT indexes are identical, and device-relative
//  track addresses are also upwardly-contiguous).  That means we can build concisely-
//  construted DAD tables, eight entries per table.
//
//  While we do that, we compare what we are creating against what already exists;
//  if it turns out that any particular DAD would be recreated to contain what it
//  already contained, then that DAD will not be marked for update -- thus, after going
//  through this process, only those DAD entries with their update flag set, need to
//  be written to the MFD.
//
//  It should be noted that a special case exists, where we might no longer need one or
//  more of the DAD entries - such an entry will be marked to-be-deleted as well as
//  updated.  Calling code must ensure that instead of rewriting the DAD sector, the
//  DAS for the sector is updated (the DAD sector *can* be zeroed out, if we decide to
//  code it that way).
//
//  Another special case exists, where we might need an additional DAD entry.  Such an
//  entry will be created in-core, with a DSADDR of zero and the update flag set.
//  During commit, this DAD must be written to a newly-allocated MFD sector (again
//  requiring update of the DAS).
//
//  Also, note that the addition or removal of DAD entries will affect the previous
//  and/or next pointers of neighboring DAD entries, which might require rewriting
//  them to the MFD even though nothing else in those DAD entries had changed.
//
//  These are issue for MFDManager to contend with - they don't apply to temporary files.
//  Never call this code for temporary files.
void
FileAllocationTable::synchronizeDADTables()
{
    lock();

    ITDADTABLES itdt = m_DADTables.begin();
    INDEX ex = 0;
    DeviceAreaDescriptor* pDAD = 0;
    for ( CITFAENTRIES itfae = m_Entries.begin(); itfae != m_Entries.end(); ++itfae )
    {
        //  If we're at the first entry for a DAD, create a new DAD
        if ( ex == 0 )
        {
            pDAD = new DeviceAreaDescriptor();
            pDAD->setFirstFileRelativeWord( itfae->first * 1792 );
            pDAD->setLastFileRelativeWord( itfae->first * 1792 );
        }

        //  Update DAD entry
        pDAD->setEntryDiskAddress( ex, itfae->second.m_DeviceTrackId * 1792 );
        pDAD->setEntryLDATIndex( ex, itfae->second.m_LDATIndex );
        pDAD->setEntryWordLength( ex, itfae->second.m_TrackCount * 1792 );
        pDAD->setLastFileRelativeWord( (itfae->first + itfae->second.m_TrackCount) * 1792 );
        ++ex;

        //  Is there a hole between this allocation and the next?
        CITFAENTRIES itfaeNext = itfae;
        ++itfaeNext;
        if ( itfaeNext != m_Entries.end() )
        {
            if ( itfae->first + itfae->second.m_TrackCount < itfaeNext->first )
            {
                //  Update another DAD entry for the hole
                TRACK_ID holeTrackId = itfae->first + itfae->second.m_TrackCount;   //  file-relative...
                TRACK_COUNT holeTrackCount = itfaeNext->first - holeTrackId;
                pDAD->setEntryDiskAddress( ex, holeTrackId * 1792 );
                pDAD->setEntryLDATIndex( ex, 0400000 );
                pDAD->setEntryWordLength( ex, holeTrackCount * 1792 );
                pDAD->setIsRemovableEntry( ex, m_IsRemovable );
                pDAD->setLastFileRelativeWord( (holeTrackId + holeTrackCount) * 1792 );
                ++ex;
            }
        }

        //  Will this be the last DAD entry in this DAD?
        //  Don't go beyond 6 entries, as the next iteration might need 2 entries.
        if ( (ex == 6) || (itfaeNext == m_Entries.end()) )
        {
            bool appendNewDAD = false;
            bool useNewDAD = false;

            //  Have we reached the end of the existing DADs?
            if ( itdt == m_DADTables.end() )
            {
                //  Yes, use the new DAD and append it.
                appendNewDAD = true;
                useNewDAD = true;
            }

            //  No - there is an existing corresponding DAD.
            //  Does it new DAD have the same content as the existing?
            else
            {
                //  If not, use the new DAD
                if ( compareDADContent( *itdt, pDAD ) == false )
                    useNewDAD = true;
            }

            //  Are we going to use the new DAD?  If so, finish fixing it up.
            if ( useNewDAD )
            {
                pDAD->setIsLastEntry( ex - 1, true );
                pDAD->setDSAddress( 0 );
                pDAD->setNextDADSector( 0 );
                pDAD->setPreviousDADSector( 0 );
                pDAD->setIsUpdated( true );

                //  Are we appending?
                if ( appendNewDAD )
                {
                    m_DADTables.push_back( pDAD );
                }
                else
                {
                    delete *itdt;
                    *itdt = pDAD;
                }
            }
            else
            {
                //  Not using the new DAD - delete it.
                delete pDAD;
            }
            pDAD = 0;

            //  Advance to the next DAD and keep going...
            ex = 0;
            if ( itdt != m_DADTables.end() )
                ++itdt;
        }
    }

    //  If we haven't reached the end of the DAD table, mark the remainder of them for deletion
    while ( itdt != m_DADTables.end() )
    {
        (*itdt)->setIsDeleted( true );
        ++itdt;
    }

    //  We are no longer unsynchronized
    m_IsUpdated = false;
    unlock();
}

