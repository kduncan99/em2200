//  DiskAllocationTable implementation



#include    "execlib.h"



//  private, protected methods

//  findContainingRegion()
//
//  Find the iterator for the entry which contains the given logical track id
DiskAllocationTable::ITREGION
DiskAllocationTable::findContainingRegion
(
    const TRACK_ID      logicalTrackId
)
{
    ITREGION itr = m_Regions.upper_bound( logicalTrackId );
    if ( itr != m_Regions.begin() )
    {
        --itr;
        if ( logicalTrackId < itr->first + itr->second->m_TrackCount )
            return itr;
    }
    return m_Regions.end();
}



//  constructors, destructors

DiskAllocationTable::DiskAllocationTable()
{}


DiskAllocationTable::DiskAllocationTable
(
    const TRACK_COUNT       diskTrackCount
)
{
    m_Regions[0] = new Region( diskTrackCount, false );
}


DiskAllocationTable::~DiskAllocationTable()
{
    for ( ITREGION itr = m_Regions.begin(); itr != m_Regions.end(); ++itr )
        delete itr->second;
}



//  public methods

//  dump()
//
//  For debugging
void
DiskAllocationTable::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const std::string&  diskIdentifier
) const
{
    stream << prefix << "DiskAllocationTable for " << diskIdentifier << std::endl;
    for ( CITREGION itr = m_Regions.begin(); itr != m_Regions.end(); ++itr )
    {
        stream << prefix << "    LogTrk:0" << std::oct << itr->first
            << " TrkCnt:0" << std::oct << itr->second->m_TrackCount
            << "  Alloc:" << (itr->second->m_Allocated ? "YES" : "NO") << std::endl;
    }
}


//  findUnallocatedRegion()
//
//  Finds an unallocated region of exactly the requested length.  Used for best-fit allocation algorithms.
bool
DiskAllocationTable::findUnallocatedRegion
(
    const TRACK_COUNT   trackCount,
    TRACK_ID* const     pFirstTrackId
) const
{
    //  Find an unallocated region of exactly the requested length
    CITREGION itr = m_Regions.begin();
    while ( itr != m_Regions.end() )
    {
        if ( !itr->second->m_Allocated && (itr->second->m_TrackCount == trackCount) )
        {
            *pFirstTrackId = itr->first;
            return true;
        }

        ++itr;
    }

    return false;
}


//  findLargerUnallocatedRegion()
//
//  Finds an unallocated region greater than or equal to the requested length.
//  Used for best-fit allocation algorithms.
bool
DiskAllocationTable::findLargerUnallocatedRegion
(
    const TRACK_COUNT   trackCount,
    TRACK_ID* const     pFirstTrackId
) const
{
    //  Find an unallocated region of exactly the requested length
    CITREGION itr = m_Regions.begin();
    while ( itr != m_Regions.end() )
    {
        if ( !itr->second->m_Allocated && (itr->second->m_TrackCount >= trackCount) )
        {
            *pFirstTrackId = itr->first;
            return true;
        }

        ++itr;
    }

    return false;
}


//  findLargestUnallocatedRegion()
//
//  Finds the largest unallocated region.
//  Used for best-fit algorithms, usually after finds of exact and larger regions fail.
bool
DiskAllocationTable::findLargestUnallocatedRegion
(
    TRACK_ID* const     pTrackId,
    TRACK_COUNT* const  pTrackCount
) const
{
    bool found = false;
    for ( CITREGION itr = m_Regions.begin(); itr != m_Regions.end(); ++itr )
    {
        if ( itr->second->m_Allocated == false )
        {
            if ( ( !found ) || ( found && (itr->second->m_TrackCount > *pTrackCount) ) )
            {
                *pTrackId = itr->first;
                *pTrackCount = itr->second->m_TrackCount;
                found = true;
            }
        }
    }

    return found;
}


//  getAllocatedTrackCount()
//
//  Returns number of tracks which are allocated
TRACK_COUNT
DiskAllocationTable::getAllocatedTrackCount() const
{
    TRACK_COUNT count = 0;
    for ( CITREGION itr = m_Regions.begin(); itr != m_Regions.end(); ++itr )
        if ( itr->second->m_Allocated )
            count += itr->second->m_TrackCount;
    return count;
}


//  initialize()
//
//  Reinitializes the object to a specific number of unallocated tracks.
//
//  Parameters:
//      diskTrackCount:     Number of tracks to be tracked
void
DiskAllocationTable::initialize
(
    TRACK_COUNT     diskTrackCount
)
{
    for ( ITREGION itr = m_Regions.begin(); itr != m_Regions.end(); ++itr )
        delete itr->second;
    m_Regions.clear();
    m_Regions[0] = new Region( diskTrackCount, false );
}


//  modifyArea()
//
//  Modifies an area of the disk (one or more tracks) as being allocated or free.
//  We expect sanity - e.g., the caller should not try to release an area containing
//  one or more already-free tracks, or to allocate an area containing one or more
//  already-allocated tracks.
//
//  Parameters:
//      trackId:        First disk-relative track in area to be modified
//      trackCount:     Number of contiguous tracks in area to be modified
//      allocateFlag:   true to allocate the area, false to free it
//
//  Returns:
//      true normally, false if we cannot do the requested action (caller should stop the exec)
bool
DiskAllocationTable::modifyArea
(
    const TRACK_ID      trackId,
    const TRACK_COUNT   trackCount,
    const bool          allocatedFlag
)
{
    //  Find containing region
    ITREGION itr = findContainingRegion( trackId );
    if ( itr == m_Regions.end() )
    {
        std::stringstream strm;
        strm << "DiskAllocationTable::modifyArea() cannot find containing region track-id=0"
            << std::oct << trackId << " trackCount=0" << std::oct << trackCount;
        SystemLog::write( strm.str() );
        return false;
    }

    //  Are we changing the entire region?  If so, just merge the regions ahead of
    //  behind this one, into one big region, deleting appropriate regions as necessary.
    //  Keep in mind we may be at the front or the back of the table.
    if ( trackCount == itr->second->m_TrackCount )
    {
        //  Merge this region into the previous region (if there is one).
        //  End up with itr referring to this previous region.
        if ( itr != m_Regions.begin() )
        {
            ITREGION itprev = itr;
            --itprev;
            itprev->second->m_TrackCount += itr->second->m_TrackCount;
            delete itr->second;
            m_Regions.erase( itr );
            itr = itprev;
        }

        //  Merge the coalesced region with the next region (if there is one).
        ITREGION itnext = itr;
        ++itnext;
        if ( itnext != m_Regions.end() )
        {
            itr->second->m_TrackCount += itnext->second->m_TrackCount;
            delete itnext->second;
            m_Regions.erase( itnext );
        }

        //  Done.
        return true;
    }

    //  Are we changing a subset of the region beginning at the head of the region?
    if ( trackId == itr->first )
    {
        //  Yes - if this is the first region, we need to create a new region
        //  to represent the remaining unchanged portion of the original region,
        //  then modify the original region to represent just the changed portion.
        if ( trackId == 0 )
        {
            m_Regions[trackId + trackCount] = new Region( itr->second->m_TrackCount - trackCount, itr->second->m_Allocated );
            m_Regions[0]->m_TrackCount = trackCount;
            m_Regions[0]->m_Allocated = allocatedFlag;
        }

        else
        {
            //  Not the first region - just move the affected area from this region to the previous.
            //  This will entail changing the index of the current region.
            ITREGION itprev = itr;
            --itprev;
            itprev->second->m_TrackCount += trackCount;

            TRACK_ID newTrackId = itr->first + trackCount;
            Region* pRegion = itr->second;
            pRegion->m_TrackCount -= trackCount;

            m_Regions.erase( itr );
            m_Regions[newTrackId] = pRegion;
        }

        //  Done
        return true;
    }

    //  Are we changing an area of the region aligned with the end of the region?
    if ( (trackId + trackCount) == (itr->first + itr->second->m_TrackCount) )
    {
        //  Yes - if this represents space at the very end of the disk pack,
        //  we need to create a new region to represent the modified area.
        ITREGION itnext = itr;
        ++itnext;
        if ( itnext == m_Regions.end() )
        {
            m_Regions[trackId] = new Region( trackCount, allocatedFlag );
            itr->second->m_TrackCount -= trackCount;
        }

        else
        {
            //  Otherwise, just move the area from the current region to the next.
            //  This will entail remapping the next region.
            itr->second->m_TrackCount -= trackCount;

            Region* pRegion = itnext->second;
            pRegion->m_TrackCount += trackCount;
            TRACK_ID newTrackId = itnext->first - trackCount;

            m_Regions.erase( itnext );
            m_Regions[newTrackId] = pRegion;
        }

        //  Done
        return true;
    }

    //  We're slicing out an area of the existing region.
    //  This means we'll need to create two new regions.
    TRACK_ID modifiedTrackId = trackId;
    TRACK_COUNT modifiedTrackCount = trackCount;
    TRACK_ID residueTrackId = trackId + trackCount;
    TRACK_COUNT residueTrackCount = itr->first + itr->second->m_TrackCount - residueTrackId;

    m_Regions[modifiedTrackId] = new Region( modifiedTrackCount, allocatedFlag );
    m_Regions[residueTrackId] = new Region( residueTrackCount, !allocatedFlag );
    itr->second->m_TrackCount -= (modifiedTrackCount + residueTrackCount);

    return true;
}

