//  DiskAllocationTable.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Tracks file allocation information for a particular disk pack / device.
//  We do it this way instead of dealing with bitmaps, because it's easier to manipulate.



#ifndef EXECLIB_DISK_ALLOCATION_TABLE
#define EXECLIB_DISK_ALLOCATION_TABLE



class   DiskAllocationTable
{
private:
    //  Region is indexed by starting logical track ID, so we don't include that in the object.
    class   Region
    {
    public:
        bool                m_Allocated;
        TRACK_COUNT         m_TrackCount;

        Region( const TRACK_COUNT   trackCount,
                const bool          allocated )
            :m_Allocated( allocated ),
            m_TrackCount( trackCount )
        {}
    };

    typedef     std::map<TRACK_ID, Region*> REGIONS;
    typedef     REGIONS::iterator           ITREGION;
    typedef     REGIONS::const_iterator     CITREGION;

    REGIONS                 m_Regions;

    ITREGION                findContainingRegion( const TRACK_ID trackId );

public:
    DiskAllocationTable();
    DiskAllocationTable( const TRACK_COUNT diskTrackCount );
    ~DiskAllocationTable();

    void                    dump( std::ostream&         stream,
                                  const std::string&    prefix,
                                  const std::string&    diskIdentifier ) const;
    bool                    findUnallocatedRegion( const TRACK_COUNT    trackCount,
                                                   TRACK_ID* const      pFirstTrackId ) const;
    bool                    findLargerUnallocatedRegion( const TRACK_COUNT  trackCount,
                                                         TRACK_ID* const    pFirstTrackId ) const;
    bool                    findLargestUnallocatedRegion( TRACK_ID* const       pTrackId,
                                                          TRACK_COUNT* const    pTrackCount ) const;
    TRACK_COUNT             getAllocatedTrackCount() const;
    void                    initialize( const TRACK_COUNT diskTrackCount );
    bool                    modifyArea( const TRACK_ID      trackId,
                                        const TRACK_COUNT   trackCount,
                                        const bool          allocatedFlag );

    //  inlines
    inline bool             allocate( const TRACK_ID        trackId,
                                      const TRACK_COUNT     trackCount )
    {
        return modifyArea( trackId, trackCount, true );
    }

    inline TRACK_COUNT      getTrackCount() const
    {
        CITREGION itr = m_Regions.end();
        --itr;
        return itr->first + itr->second->m_TrackCount;
    }

    inline bool             release( const TRACK_ID         trackId,
                                     const TRACK_COUNT      trackCount )
    {
        return modifyArea( trackId, trackCount, false );
    }

    //  static inlines
    static inline bool      isContiguous( const ITREGION    itprev,
                                            const ITREGION  itnext )
    {
        return (itprev->second->m_Allocated == itnext->second->m_Allocated)
            && (itprev->first + itprev->second->m_TrackCount == itnext->first);
    }
};



#endif
