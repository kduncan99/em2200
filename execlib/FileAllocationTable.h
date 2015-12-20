//  Header file for file allocation table
//  Copyright (c) 2015 by Kurt Duncan
//
//  This class encapsulates information derived from DAD tables for configured disk files,
//  and provides in-core address translation for cataloged and temporary disk files.



#ifndef     EXECLIB_FILE_ALLOCATION_TABLE
#define     EXECLIB_FILE_ALLOCATION_TABLE



class   FileAllocationTable : public Lockable
{
public:
    class   DeviceAreaDescriptor
    {
        //  Describes file allocation, allowing us to map a file-relative track ID to a disk-relative track ID.
        //  Per system architecture, this structure is an extension of the 28-word DAD sector in the MFD.
        //
        //  Format of a DAD sector is:
        //      +00,W       DSADDR of next DAD sector or 0400000000000
        //      +01,W       DSADDR of previous DAD sector, or of main item sector 0
        //      +02,W       File-relative word address of the first word tracked in this DAD sector
        //      +03,W       File-relative word address of the last word tracked in this DAD sector, plus 1
        //      +04         Up to 8 3-word DAD entries:
        //          entry+00,W  DRWA of this region on the disk
        //          entry+01,W  Word length of this region
        //          entry+02,H1 DAD Flags
        //              Bit 15: This is the last entry in this table
        //              Bit 16: This file is on removable disk
        //          entry+02,H2 LDATIndex   Index into fixed disk or into pack table for removable; 0400000 indicates no allocation
        //
        //  DADs are a pathological nightmare to deal with in terms of updating allocations.  So we don't.
        //  Instead, we load them into the FAT, then use the content thereof to build the FileAllocationEntry's
        //  which are easier to deal with.  When we update allocations, we do so through the FAE's, and only fix up
        //  the DADs when we're getting ready to write them back out to the MFD.
    private:
        DSADDR                  m_DSAddress;        //  DSADDR of this DAD in the MFD
        Word36                  m_Data[28];         //  DAD sector
        bool                    m_IsDeleted;        //  This DAD is no longer needed, and should be deallocated
        bool                    m_IsUpdated;        //  This DAD has been updated and needs to be written to MFD

        inline Word36*          entryAddress( const INDEX ex )              { return &(m_Data[(3 * ex) + 4]); }

    public:
        DeviceAreaDescriptor()
            :m_DSAddress( 0 ),
            m_IsDeleted( false ),
            m_IsUpdated( false )
        {}

        inline const Word36*    getData() const                             { return m_Data; }
        inline DSADDR           getDSAddress() const                        { return m_DSAddress; }
        inline const Word36*    getEntry( const INDEX ex ) const            { return &(m_Data[(3 * ex) + 4]); }
        inline DRWA             getEntryDiskAddress( const INDEX ex ) const { return getEntry( ex )[0].getW(); }
        inline LDATINDEX        getEntryLDATIndex( const INDEX ex ) const   { return getEntry( ex )[2].getH2(); }
        inline WORD_COUNT       getEntryWordLength( const INDEX ex ) const  { return getEntry( ex )[1].getW(); }
        inline WORD_ID          getFirstFileRelativeWord() const            { return m_Data[2].getW(); }
        inline WORD_ID          getLastFileRelativeWord() const             { return m_Data[3].getW(); }
        inline DSADDR           getNextDADSector() const                    { return static_cast<DSADDR>(m_Data[0].getW()); }
        inline DSADDR           getPreviousDADSector() const                { return static_cast<DSADDR>(m_Data[1].getW()); }
        inline bool             isDeleted() const                           { return m_IsDeleted; }
        inline bool             isLastEntry( const INDEX ex ) const         { return (getEntry(ex)[2].getH1() & 04) != 0; }
        inline bool             isRemovableEntry( const INDEX ex ) const    { return (getEntry(ex)[2].getH1() & 02) != 0; }
        inline bool             isUpdated() const                           { return m_IsUpdated; }

        inline void setDSAddress( const DSADDR address )                { m_DSAddress = address; }

        inline void setEntryDiskAddress
        (
            const INDEX         ex,
            const DRWA          address
        )
        {
            entryAddress( ex )[0].setW( address );
        }

        inline void setEntryLDATIndex
        (
            const INDEX         ex,
            const LDATINDEX     ldatIndex
        )
        {
            entryAddress( ex )[2].setH2( ldatIndex );
        }

        inline void setEntryWordLength
        (
            const INDEX         ex,
            const WORD_COUNT    wordCount
        )
        {
            entryAddress( ex )[1].setW( wordCount );
        }

        inline void setIsLastEntry
        (
            const INDEX         ex,
            const bool          flag
        )
        {
            if ( flag )
                entryAddress( ex )[2].logicalOr( 04000000l );
            else
                entryAddress( ex )[2].logicalAnd( 0777773777777l );
        }

        inline void setIsRemovableEntry
        (
            const INDEX         ex,
            const bool          flag
        )
        {
            if ( flag )
                entryAddress( ex )[2].logicalOr( 02000000l );
            else
                entryAddress( ex )[2].logicalAnd( 0777775777777l );
        }

        inline void setIsDeleted( const bool flag )                     { m_IsDeleted = flag; }
        inline void setIsUpdated( const bool flag )                     { m_IsUpdated = flag; }
        inline void setFirstFileRelativeWord( const WORD_ID wordId )    { m_Data[2].setW( wordId ); }
        inline void setLastFileRelativeWord( const WORD_ID wordId )     { m_Data[3].setW( wordId ); }
        inline void setNextDADSector( const DSADDR address )            { m_Data[0].setW( address ); }
        inline void setPreviousDADSector( const DSADDR address )        { m_Data[1].setW( address ); }
    };

    typedef     std::list<DeviceAreaDescriptor*>            DADTABLES;
    typedef     DADTABLES::iterator                         ITDADTABLES;
    typedef     DADTABLES::const_iterator                   CITDADTABLES;

    class   FileAllocationEntry
    {
    public:
        LDATINDEX               m_LDATIndex;        //  Identifies the pack containing this allocation
        TRACK_ID                m_DeviceTrackId;    //  Device-relative track id of the first track in this allocation
        TRACK_COUNT             m_TrackCount;       //  Number of tracks in this allocation

        FileAllocationEntry( const TRACK_COUNT  trackCount,
                             const LDATINDEX    ldatIndex,
                             const TRACK_ID     deviceTrackId )
            :m_LDATIndex( ldatIndex ),
            m_DeviceTrackId( deviceTrackId ),
            m_TrackCount( trackCount )
        {}

        FileAllocationEntry()
            :m_LDATIndex( 0 ),
            m_DeviceTrackId( 0 ),
            m_TrackCount( 0 )
        {}
    };

    typedef     std::map<TRACK_ID, FileAllocationEntry>     FAENTRIES;      //  key is file-relative track-id
    typedef     FAENTRIES::iterator                         ITFAENTRIES;
    typedef     FAENTRIES::const_iterator                   CITFAENTRIES;

private:
    DADTABLES               m_DADTables;            //  MFD entries which live on disk
    FAENTRIES               m_Entries;              //  in-core entries which are easier to deal with
    bool                    m_IsRemovable;
    bool                    m_IsUpdated;            //  DADs are out-of-sync, and need fixed and re-written to MFD
    DSADDR                  m_MainItem0Addr;

    static bool             compareDADContent( const DeviceAreaDescriptor* const    pDAD1,
                                               const DeviceAreaDescriptor* const    pDAD2 );


public:
    FileAllocationTable( const DSADDR   mainItem0Address,
                         const bool     isRemovable )
        :m_IsRemovable( isRemovable ),
        m_IsUpdated( false ),
        m_MainItem0Addr( mainItem0Address )
    {}

    ~FileAllocationTable()
    {
        clear();
    }

    bool                allocated( const TRACK_ID       fileRelativeFirstTrackId,
                                   const TRACK_COUNT    trackCount,
                                   const LDATINDEX      ldatIndex,
                                   const TRACK_ID       deviceRelativeFirstTrackId );
    void                buildFileAllocationEntries();
    void                clear();
    bool                convertTrackId( const TRACK_ID      fileRelativeTrackId,
                                        LDATINDEX* const    ldatIndex,
                                        TRACK_ID* const     deviceRelativeTrackId ) const;
    void                dump( std::ostream&         stream,
                              const std::string&    prefix,
                              const std::string&    fileIdentifier ) const;
    TRACK_COUNT         getAllocatedTrackCount() const;
    void                getFileAllocationEntries( const TRACK_ID    firstTrackId,
                                                  const TRACK_COUNT trackCount,
                                                  FAENTRIES* const  pFileAllocationEnties ) const;
    TRACK_ID            getHighestTrackAssigned() const;
    bool                released( const TRACK_ID    fileRelativeFirstTrackId,
                                  const TRACK_COUNT trackCount );
    void                synchronizeDADTables();

    DADTABLES&          getDADTables()                                  { return m_DADTables; }
    inline const FileAllocationEntry&
                        getFirstAllocation() const                      { return m_Entries.begin()->second; }
    inline DSADDR       getMainItem0Addr() const                        { return m_MainItem0Addr; }
    inline bool         isEmpty() const                                 { return m_Entries.empty(); }
    inline bool         isRemovable() const                             { return m_IsRemovable; }
    inline bool         isUpdated() const                               { return m_IsUpdated; }
    inline void         setMainItem0Addr( const DSADDR address )        { m_MainItem0Addr = address; }
};



#endif
