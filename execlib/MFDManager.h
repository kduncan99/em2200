//  MFDManager.h
//
//  MFD manager for Emulated Exec
//  Handles all MFD-related calisthenics, as well as file allocation / track allocation etc.
//
//  Some implementation notes:
//      LDATINDEX is shared between fixed and removable devices.  Since LDATINDEX is (in other areas)
//      limited to 12 bits, this means we have a maximum number of 4096 packs we can keep track of.
//      That's a lot.
//      Architectural documentation is somewhat skimpy with regards to how LDAT indexes are assigned to
//      removable packs, other than mentioning that the value is actually the index into a pack table.
//      What we do *not* know, is whether this means that a fixed pack can have an identical LDATINDEX
//      as some other removable pack - it seems possible.
//      For our purposes, we will limit the number of fixed packs and removable packs to 2048 each.
//      LDATINDEX value for fixed packs will vary from 0 to 03777, while for removable packs it will
//      vary from 04000 to 07777.  Thus, we can use common routines in many instances, secure in the
//      knowledge that there will never be an LDATINDEX collision between a fixed and a removable pack.


//  TODO:RECOV Move the following down to recovery, and use it as an outline for developing that stuff.
//  For recovery boot
//      (Configurator provides previous session data)
//      (device manager sets device status based on previous session)
//      Notify user if config has changed since last session
//      if JK3 is clear, report stop information for previous session
//      depending on JK, let user modify config
//      readDiskLabels()
//          read disk labels for all disks which are SU or UP to build m_BootDiskInfo
//      countFixedPacks()
//          (BootActivity will report number of fixed MS - if different than previous, prompt user)
//      loadFixedPackInfo()
//          iterate over BootDiskInfo
//              recover LDAT indices
//              filter out freshly-prepped FIXED disks
//              build PackInfo for all FIXED disks
//              complain and crash on pack-name conflict
//      load initial directory blocks from fixed packs
//      load extended directory blocks from fixed packs
//      manually assign SYS$*MFDF$$
//          update main item indicating assignment
//          (because we don't have FAC item, Facilities cannot call us to find the info)
//          commit
//      Use normal IO to build the search item lookup table
//          check for inconsistencies and crash if we find them
//      Use normal IO to load internal track allocation tables (connect to PackInfo)
//      Drop all files in to-be state (yes? no? more thought needed)
//      Removable stuff here... lots to do (but what?)
//      What else?  TIP, Audit trails, etc - which we haven't yet implemented



#ifndef     EXECLIB_MFD_MANAGER_H
#define     EXECLIB_MFD_MANAGER_H



#include    "ConsoleManager.h"
#include    "DeviceManager.h"
#include    "DiskAllocationTable.h"
#include    "DiskFacilityItem.h"
#include    "ExecManager.h"
#include    "FileAllocationTable.h"



class   MFDManager : public ExecManager
{
public:
    typedef     std::list<DSADDR>                   DSADDRLIST;
    typedef     DSADDRLIST::iterator                ITDSADDRLIST;
    typedef     DSADDRLIST::const_iterator          CITDSADDRLIST;

    typedef     std::set<DSADDR>                    DSADDRSET;
    typedef     DSADDRSET::iterator                 ITDSADDRSET;
    typedef     DSADDRSET::const_iterator           CITDSADDRSET;

    typedef     std::vector<DSADDR>                 DSADDRVECTOR;

    //  Public data types ------------------------------------------------------
    enum FileType
    {
        FILETYPE_NONE,
        FILETYPE_MASS_STORAGE,
        FILETYPE_TAPE,
        FILETYPE_REMOVABLE,
    };

    enum	TipInfo
	{
		TIPINFO_NON_TIP,
		TIPINFO_DUPLEX_LEG_1,
		TIPINFO_DUPLEX_LEG_2,
		TIPINFO_SIMPLEX,
	};

	enum    Status
    {
        MFDST_SUCCESSFUL,               //  all went well
        MFDST_DEVICE_NOT_ACCESSIBLE,    //  device is DN or RV, or it is UP or SU but there is no accessible path
        MFDST_FILE_ON_PRINT_QUEUE,      //  file cannot be dropped because it is on the print queue
        MFDST_FRESHLY_PREPPED,          //  registration not allowed for freshly-prepped fixed on recovery boot
        MFDST_INTERNAL_ERROR,           //  other internal error
        MFDST_IO_ERROR,                 //  got a channel error, possibly a device error
        MFDST_LDAT_CONFLICT,            //  on recovery, there is a problem with a pack LDAT
        MFDST_MAX_GRANULES_EXCEEDED,    //  allocation request failed due to max grans being exceeded
        MFDST_NO_PATH,                  //  No path exists for direct disk IO
        MFDST_NOT_FOUND,                //  Whatever the caller asked about, we couldn't find it
        MFDST_OPERATOR_ABORTED_ACTION,  //  Presented with a YN query, operator elected to abort an operation
        MFDST_OUT_OF_SPACE,             //  the pack (or the fixed pool) has no more space
        MFDST_PACK_NAME_CONFLICT,       //  fixed pack brought online with a pack name matching another pack already known
        MFDST_PACK_NOT_FORMATTED,       //  An attempt to prep the pack failed, as the pack is not "low-level formatted"
        MFDST_PACK_NOT_PREPPED,         //  pack is not prepped, or needs to be reprepped
        MFDST_SECTOR1_CONFLICT,         //  something is wrong in MFD sector 1 for the pack
        MFDST_TERMINATING,              //  function stopped prematurely due to exec shutdown
        MFDST_UNSUPPORTED_PACK,         //  Pack is too large
    };


    //  Reflects all that is known about a particular file cycle based on
	//  the existing MFD main file item sectors 0 through {n}.
	class	FileCycleInfo
	{
	public:
		class	BackupInfo
		{
		public:
			TDate                   m_TDateBackup;
			bool                    m_UnloadedAtBackup;
			COUNT32                 m_MaxBackupLevels;
			COUNT32                 m_CurrentBackupLevels;
			COUNT32                 m_NumberOfBlocks;
			COUNT32                 m_StartingFilePosition;
			LSTRING                 m_BackupReels;

			BackupInfo()
				:m_UnloadedAtBackup(false),
				m_MaxBackupLevels(0),
				m_CurrentBackupLevels(0),
				m_NumberOfBlocks(0),
				m_StartingFilePosition(0)
			{}
		};

		DSADDRVECTOR            m_DirectorySectorAddresses;
		SuperString             m_Qualifier;
		SuperString             m_FileName;
		SuperString             m_ProjectId;
		SuperString             m_AccountId;
		SuperString             m_AssignMnemonic;
		UINT16                  m_AbsoluteCycle;
		bool                    m_ToBeCataloged;
		bool                    m_ToBeDropped;
		bool                    m_ToBeReadOnly;
		bool                    m_ToBeWriteOnly;
		bool                    m_DirectoryDisabled;            //  Something wrong with the directory entries for this cycle
		bool                    m_WrittenToDisabled;            //  File was assigned and written to when the system crashed
		bool                    m_InaccessibleBackupDisabled;   //  Rolled out, and backup tape is inaccessible
		bool                    m_BackedUp;
		bool                    m_Guarded;
		bool                    m_Private;
		bool                    m_ExclusiveUse;
		bool                    m_ReadInhibited;
		bool                    m_WriteInhibited;
		bool                    m_Queued;
		COUNT32                 m_CumulativeAssignCount;
		COUNT32                 m_CurrentAssignCount;
		TDate                   m_TDateCurrentAssignment;
		TDate                   m_TDateCataloged;

		BackupInfo              m_BackupInfo;
		TipInfo                 m_TipInfo;

		virtual ~FileCycleInfo(){};
	};


	class MassStorageFileCycleInfo : public FileCycleInfo
	{
	public:
		TDate                   m_TDateFirstWriteAfterBackup;
		bool                    m_Unloaded;
		bool                    m_SaveOnCheckpoint;
		bool                    m_LargeFile;
		bool                    m_WrittenTo;
		bool                    m_StoreThrough;
		bool                    m_PositionGranularity;
		bool                    m_WordAddressable;
		bool                    m_AssignedToCommonNameSection;
		bool                    m_UnloadInhibited;
		COUNT32                 m_InitialReserve;
		COUNT32                 m_MaxGranules;
		COUNT32                 m_HighestGranuleAssigned;
		COUNT32                 m_HighestTrackWritten;
	};


	class TapeFileCycleInfo : public FileCycleInfo
	{
	public:
		TapeBlockNumbering      m_BlockNumbering;   //  from features
		TapeDataCompression     m_DataCompression;  //  from features
		TapeDensity             m_Density;
		TapeFormat              m_Format;
		TapeParity              m_Parity;
		TapeType                m_Type;             //  (from MTAPOP)
		UINT8                   m_NoiseConstant;
		bool                    m_JOption;          //  cataloged with J-option, from MTAPOP
		LSTRING                 m_Reels;
	};


    //  Reflects all that is known about a fileset based on the MFD lead item sectors 0 and 1.
    //  Does NOT include any information obtainable only via main items.
    class FileSetInfo
    {
    public:
        class CycleEntry
        {
        public:
            bool                    m_Exists;
            bool                    m_ToBeCataloged;
            bool                    m_ToBeDropped;
            INT16                   m_RelativeCycle;
            UINT16                  m_AbsoluteCycle;
            DSADDR                  m_MainItem0Addr;

            CycleEntry( )
                :m_Exists( false ),
                m_ToBeCataloged( false ),
                m_ToBeDropped( false ),
                m_RelativeCycle( 0 ),
                m_AbsoluteCycle( 0 ),
                m_MainItem0Addr( 0 )
            {}
        };

        DSADDR                      m_MFDIdentifier;    //  Something which uniquely identifies the fileset
                                                        //      currently, we use the lead item sector 0 address
        DSADDRVECTOR                m_DSAddresses;      //  Vector of lead item addresses we used to create this object
        SuperString                 m_Qualifier;
        SuperString                 m_Filename;
        SuperString                 m_ProjectId;
        SuperString                 m_ReadKey;
        SuperString                 m_WriteKey;
        FileType                    m_FileType;
        COUNT32                     m_MaxRange;
        bool                        m_Guarded;
        bool                        m_PlusOneCycleExists;
        std::vector<CycleEntry>     m_CycleEntries;

        FileSetInfo( )
            :m_FileType( FILETYPE_MASS_STORAGE ),
            m_MaxRange( 0 ),
            m_Guarded( false ),
            m_PlusOneCycleExists( false )
        {}
    };

    class   FileSetInfoList : public std::list<FileSetInfo*>
    {
    public:
        ~FileSetInfoList()                  { clear(); }

        void clear()
        {
            for ( iterator it = begin(); it != end(); ++it )
                delete *it;
        }
    };

    //  Represents the status of a request
    class   Result
    {
    public:
        Status                      m_Status;
        ChannelModule::Status       m_ChannelStatus;
        Device::IoStatus            m_DeviceStatus;
        SYSTEMERRORCODE             m_SystemErrorCode;

        Result()
        {
            m_Status = MFDST_SUCCESSFUL;
            m_ChannelStatus = ChannelModule::Status::SUCCESSFUL;
            m_DeviceStatus = Device::IoStatus::SUCCESSFUL;
            m_SystemErrorCode = 0;
        }
    };

    class   PackInfo
    {
        //  To be used in a map, keyed by LDATINDEX (as soon as it is available)
    public:
        DeviceManager::DEVICE_ID    m_DeviceId;                 //  If mounted, this is the DEVICE_ID for the corresponding device
        SECTOR_ID                   m_DirectoryTrackAddress;    //  device-relative first directory track sector address
        DiskAllocationTable         m_DiskAllocationTable;      //  Allocations of HMBT, temporary, and cataloged files
        COUNT                       m_AssignCount;              //  Assign count against this pack
        bool                        m_IsFixed;                  //  Pack is fixed - this does NOT mean it is incorporated
                                                                //      into the fixed pool - the device might be DN after
                                                                //      operator prepped, started to UP it, then changed his mind.
        bool                        m_InFixedPool;              //  Pack is part of fixed pool.
        bool                        m_IsMounted;                //  This pack is mounted on a device
        LDATINDEX                   m_LDATIndex;
        SuperString                 m_PackName;
        PREP_FACTOR                 m_PrepFactor;
        WORD_COUNT                  m_S0S1HMBTPadWords;         //  Length of S0+S1+HMBT+padding_to_block_boundary
        WORD_COUNT                  m_SMBTWords;                //  Length of SMBT in words (not padded)
        TRACK_COUNT                 m_TotalTracks;              //  Total tracks, incl. initial track allocation

        PackInfo()
            :m_DeviceId( 0 ),
            m_DirectoryTrackAddress( 0 ),
            m_AssignCount( 0 ),
            m_IsFixed( false ),
            m_InFixedPool( false ),
            m_IsMounted( false ),
            m_LDATIndex( 0 ),
            m_PrepFactor( 0 ),
            m_S0S1HMBTPadWords( 0 ),
            m_SMBTWords( 0 ),
            m_TotalTracks( 0 )
        {}
    };

private:
    typedef     std::list<PackInfo*>                            PACKINFOLIST;
    typedef     PACKINFOLIST::iterator                          ITPACKINFOLIST;
    typedef     PACKINFOLIST::const_iterator                    CITPACKINFOLIST;

    typedef     std::map<LDATINDEX, PackInfo*>                  PACKINFOMAP;
    typedef     PACKINFOMAP::iterator                           ITPACKINFOMAP;
    typedef     PACKINFOMAP::const_iterator                     CITPACKINFOMAP;


    //  Dictionary of FILEALLOCATIONTABLE's, keyed by the DSADDR of the file's main item 0 sector
    typedef     std::map<DSADDR, FileAllocationTable*>          FILEALLOCATIONDICTIONARY;
    typedef     FILEALLOCATIONDICTIONARY::iterator              ITFILEALLOCATIONDICTIONARY;
    typedef     FILEALLOCATIONDICTIONARY::const_iterator        CITFILEALLOCATIONDICTIONARY;


    typedef     std::map<DSADDR, Word36*>                       DIRECTORYCACHE;
    typedef     DIRECTORYCACHE::iterator                        ITDIRECTORYCACHE;
    typedef     DIRECTORYCACHE::const_iterator                  CITDIRECTORYCACHE;

    typedef     std::map<TRACK_ID, BLOCK_ID>                    DIRECTORYTRACKIDMAP;
    typedef     DIRECTORYTRACKIDMAP::iterator                   ITDIRECTORYTRACKIDMAP;
    typedef     DIRECTORYTRACKIDMAP::const_iterator             CITDIRECTORYTRACKIDMAP;


    //  private data
    PACKINFOLIST                        m_BootPackInfo;                 //  Created by readDiskLabels(), consumed by initialize() or recover()
    DIRECTORYCACHE                      m_DirectoryCache;               //  Where we cache directory sectors we're working on
    DIRECTORYTRACKIDMAP                 m_DirectoryTrackIdMap;          //  maps directory-relative track IDs to device-relative block IDs
    ConsoleManager* const               m_pConsoleManager;              //  convenience pointer
    DeviceManager* const                m_pDeviceManager;               //  convenience pointer
    FILEALLOCATIONDICTIONARY            m_FileAllocationDictionary;     //  FAT's for all assigned files
    COUNT32                             m_LookupTableSize;              //  from DCLUTS
    std::string                         m_OverheadAccountId;            //  from OVRACC
    std::string                         m_OverheadUserId;               //  from OVRUSR
    PACKINFOMAP                         m_PackInfo;                     //  Maps LDATINDEX to information known about the pack
    DSADDRVECTOR                        m_SearchItemLookupTable;        //  Value of zero indicates no search item
    DSADDRSET                           m_UpdatedSectors;               //  DSADDRs of updated directory sectors

    //  private methods
    Result                      allocateDirectorySector( Activity* const    pActivity,
                                                         const LDATINDEX    preferredLDATIndex,
                                                         DSADDR* const      pDSAddress );
    Result                      allocateDirectorySectorOnDAS( Activity* const   pActivity,
                                                              const DSADDR      dasAddr,
                                                              DSADDR* const     pDSAddress );
    Result                      allocateDirectorySectorOnPack( Activity* const      pActivity,
                                                               const LDATINDEX      ldatIndex,
                                                               DSADDR* const        pDSAddress );
    Result                      allocateDirectoryTrack( Activity* const     pActivity,
                                                        const LDATINDEX     preferredLDATIndex );
    Result                      allocateFixedTracks( Activity* const        pActivity,
                                                     const PACKINFOLIST&    packList,
                                                     const TRACK_COUNT      tracksRequested,
                                                     const bool             temporaryFile,
                                                     LDATINDEX* const       pAllocatedLDATIndex,
                                                     TRACK_ID* const        pDeviceTrackId,
                                                     TRACK_COUNT* const     pTracksAllocated );
    Result                      bringFixedPackOnline( Activity* const                       pActivity,
                                                      const DeviceManager::NodeEntry* const pNodeEntry,
                                                      PackInfo* const                       pPackInfo );
    LDATINDEX                   chooseFixedLDATIndex();
    Result                      commitMFDUpdates( Activity* const pActivity );
    Result                      deallocateDirectorySector( Activity* const  pActivity,
                                                           const DSADDR     dsAddress );
    Result                      deallocateDirectorySectors( Activity* const     pActivity,
                                                            const DSADDRSET&    dsAddresses );
    Result                      deallocateFixedTracks( Activity* const      pActivity,
                                                       const LDATINDEX      ldatIndex,
                                                       const TRACK_ID       trackId,
                                                       const TRACK_COUNT    trackCount,
                                                       const bool           temporaryFile );
    Result                      directDiskIo( Activity* const                   pActivity,
                                              const DeviceManager::DEVICE_ID    deviceId,
                                              const ChannelModule::Command      channelCommand,
                                              const BLOCK_ID                    blockId,
                                              const WORD_COUNT                  wordCount,
                                              Word36* const                     pBuffer ) const;
    bool                        directDiskIoError( const DeviceManager::DEVICE_ID   deviceId,
                                                   const ChannelModule::Command     channelCommand,
                                                   const BLOCK_ID                   blockId,
                                                   const WORD_COUNT                 wordCount,
                                                   const DeviceManager::Path* const pPath,
                                                   const Result&                    result ) const;
    Result                      dropDADTables( Activity* const      pActivity,
                                               const Word36* const  pMainItem1 );
    Result                      dropFileCycleInternal( Activity* const  pActivity,
                                                       const DSADDR     mainItem0Addr,
                                                       const UINT8      fileType );
    Result                      dropFileSetInternal( Activity* const    pActivity,
                                                     const DSADDR       leadItem0Addr );
    Result                      dropReelTables( Activity* const     pActivity,
                                                const Word36* const pMainItem1 );
    void                        dumpMFDDirectory( std::ostream&     stream );
    void                        dumpMFDDirectoryLeadItem( std::ostream& stream,
                                                          const DSADDR  leadItem0Addr );
    void                        dumpMFDDirectoryMainItem( std::ostream&     stream,
                                                          const FileType    fileType,
                                                          const int         relativeCycle,
                                                          const DSADDR      mainItem0Addr );
    Result                      establishLookupEntry( Activity* const       pActivity,
                                                      const Word36* const   pQualifier,
                                                      const Word36* const   pFileName,
                                                      const DSADDR          leadItemAddr );
    void                        getConfigData();
    Result                      getDiskDeviceInfo( Activity* const                          pActivity,
                                                   const DeviceManager::DEVICE_ID           deviceId,
                                                   DeviceManager::DiskDeviceInfo36** const  ppInfo );
    Result                      getDiskDeviceValues( Activity* const                pActivity,
                                                     const DeviceManager::DEVICE_ID deviceId,
                                                     BLOCK_COUNT* const             pBlockCount = 0,
                                                     BLOCK_SIZE* const              pBlockSize = 0,
                                                     bool* const                    pIsMounted = 0,
                                                     bool* const                    pIsReady = 0,
                                                     bool* const                    pIsWriteProtected = 0 );
    Result                      getFileSetInfo( const DSADDR        leadItemAddr0,
                                                FileSetInfo* const  pFileSetInfo ) const;
    DSADDR                      getLeadItemAddress( const Word36* const pQualifier,
                                                    const Word36* const pFileName ) const;
    DSADDR                      getSearchItemAddress( const Word36* const   pQualifier,
                                                      const Word36* const   pFileName ) const;
    DSADDR                      getSearchItemAddress( const SuperString&    qualifier,
                                                      const SuperString&    filename ) const;
    Result                      initializeAssignMFD( Activity* const            pActivity,
                                                     const DSADDR               pMFDMainItem0Addr,
                                                     FileAllocationTable* const pMFDFat );
    Result                      initializeCatalogMFD( Activity* const               pActivity,
                                                      DSADDR* const                 pMFDMainItem0Addr,
                                                      FileAllocationTable** const   ppMFDFat );
    Result                      initializeCatalogMFDLeadItem( const DSADDR  leadItemAddr,
                                                              const DSADDR  mainItem0Addr );
    Result                      initializeCatalogMFDMainItem0( const DSADDR mainItem0Addr,
                                                               const DSADDR leadItemAddr,
                                                               const DSADDR mainItem1Addr );
    Result                      initializeCatalogMFDMainItem1( const DSADDR mainItem1Addr,
                                                               const DSADDR mainItem0Addr );
    Result                      initializeCatalogMFDSearchItem( const DSADDR    searchItemAddr,
                                                                const DSADDR    leadItem0Addr );
    Result                      initializeCreateMFDDADTable( Activity* const                pActivity,
                                                             const DSADDR                   mainItem0Addr,
                                                             FileAllocationTable** const    ppMFDFat );
    Result                      initializeFixedPack( Activity* const    pActivity,
                                                     ITPACKINFOMAP      itPackInfo );
    Result                      initializeFixedPacks( Activity* const pActivity );
    void                        initializeLoadPackInfo( Activity* const         pActivity,
                                                        const PACKINFOLIST&     packInfoList );
    Result                      insertMainItem( Activity* const pActivity,
                                                const DSADDR    leadItem0Address,
                                                const DSADDR    mainItem0Address,
                                                const UINT16    absoluteCycle,
                                                const bool      guardedFile );
    Result                      internalPrep( Activity* const                   pActivity,
                                              const DeviceManager::DEVICE_ID    deviceId,
                                              const bool                        fixedFlag,
                                              const std::string&                packLabel );
    Result                      internalPrepCreateDASSector( Activity* const                 pActivity,
                                                             const DeviceManager::DEVICE_ID  deviceId,
                                                             const PREP_FACTOR               prepFactor,
                                                             const DRWA                      dasTrackWordAddress ) const;
    Result                      loadDirectoryTrackIntoCache( Activity* const    pActivity,
                                                             PackInfo* const    pPackInfo,
                                                             DSADDR             firstDSAddress,
                                                             BLOCK_ID           firstBlockId );
    Result                      loadFileAllocations( Activity* const            pActivity,
                                                     const DSADDR               mainItem0Addr,
                                                     FileAllocationTable* const pFAT );
    Result                      loadFixedPackAllocationTable( Activity* const   pActivity,
                                                              CITPACKINFOMAP    itPackInfo );
    Result                      loadFixedPackAllocationTables( Activity* const  pActivity );
    Result                      readDiskLabel( Activity* const                  pActivity,
                                               const DeviceManager::DEVICE_ID   deviceId,
                                               PackInfo** const                 ppPackInfo );
    Result                      readDiskLabels( Activity* const     pActivity,
                                                PACKINFOLIST* const pPackInfoList );
#if 0 //TODO:RECOV
    Result                      recoverLookupTable();
    Result                      recoverMFDFat();
#endif
    Result                      removeLookupEntry( Activity* const      pActivity,
                                                    const Word36* const pQualifier,
                                                    const Word36* const pFileName );
    Result                      setSMBTAllocated( Activity* const       pActivity,
                                                  const LDATINDEX       ldatIndex,
                                                  const TRACK_ID        trackId,
                                                  const TRACK_COUNT     trackCount,
                                                  const bool            allocated,
                                                  const bool            updateHMBT );
    bool                        stageDirectorySector( const DSADDR      directorySectorAddress,
                                                      Word36** const    ppSector,
                                                      Result* const     pResult ) const;
    bool                        stageDirectorySector( const DSADDR      directorySectorAddress,
                                                      const bool        setUpdated,
                                                      Word36** const    ppSector,
                                                      Result* const     pResult );
    bool                        stageLeadItems( const DSADDR        leadItem0Addr,
                                                DSADDR* const       pLeadItem1Addr,
                                                Word36** const      ppSector0,
                                                Word36** const      ppSector1,
                                                Result* const       pResult ) const;
    bool                        stageLeadItems( const DSADDR        leadItem0Addr,
                                                const bool          updateFlag,
                                                DSADDR* const       pLeadItem1Addr,
                                                Word36** const      ppSector0,
                                                Word36** const      ppSector1,
                                                Result* const       pResult );
    bool                        stageMainItems( const DSADDR        mainItem0Addr,
                                                const bool          updateFlag,
                                                DSADDR* const       pMainItem1Addr,
                                                Word36** const      ppSector0,
                                                Word36** const      ppSector1,
                                                Result* const       pResult );
    void                        stopExecOnResultStatus( const Result&   result,
                                                        const bool      allowIoError ) const;
    Result                      updateGranulesInfo( Activity* const     pActivity,
                                                    const DSADDR        mainItem0Addr,
                                                    const UINT32        initialGranules,
                                                    const UINT32        maximumGranules,
                                                    const UINT32        highestGranuleAssigned,
                                                    const UINT32        highestTrackWritten );
    Result                      updateGuardedBit( Activity* const       pActivity,
                                                  const DSADDR          leadItem0Addr,
                                                  Word36* const         pLeadItem0,
                                                  const Word36* const   pLeadItem1 );
    Result                      writeDADUpdates( Activity* const            pActivity,
                                                 FileAllocationTable* const pFileAllocationTable );

    //  private inlines
    inline INDEX getLookupTableHashIndex( const Word36* const   pQualifier,
                                            const Word36* const pFileName ) const
    {
#if 1   //  TODO:DEBUG
        INDEX result = pQualifier[0].getH1() + (pQualifier[0].getH2() << 4);
        result += (pQualifier[1].getH1() << 8) + (pQualifier[1].getH2() << 12);
        result += pFileName[0].getH1() + (pFileName[0].getH2() << 4);
        result += (pFileName[1].getH1() << 8) + (pFileName[1].getH2() << 12);
#else   //  below is for testing pathological conditions for search item algorithms
        INDEX result = pQualifier[0].getH1() + pQualifier[0].getH2();
        result += pQualifier[1].getH1() + pQualifier[1].getH2();
#endif
        return result % m_LookupTableSize;
    }

    inline static Word36* getMainItemLinkPointer( const INDEX       entryIndex,   // most current is 0, -31 is 31, etc
                                                    Word36* const   pLeadItem0,
                                                    Word36* const   pLeadItem1 )
    {
        if ( entryIndex < 17 )
            return pLeadItem0 + 013 + entryIndex;
        return pLeadItem1 + 01 + ( entryIndex - 17 );
    }

    inline static const Word36* getMainItemLinkPointer( const INDEX         entryIndex,   // most current is 0, -31 is 31, etc
                                                        const Word36* const pLeadItem0,
                                                        const Word36* const pLeadItem1 )
    {
        if ( entryIndex < 17 )
            return pLeadItem0 + 013 + entryIndex;
        return pLeadItem1 + 01 + ( entryIndex - 17 );
    }

    //  private statics
    static void                 buildSector0ForLargeDisks( Word36* const        pSector0,
                                                           const SECTOR_ID      firstDirectoryTrackAddress,
                                                           const TRACK_COUNT    directoryTracks );
    static void                 buildSector0ForSmallDisks( Word36* const        pSector0,
                                                           const SECTOR_ID      firstDirectoryTrackAddress,
                                                           const WORD_COUNT     hmbtLength,
                                                           const WORD_COUNT     smbtLength );
    static void                 buildEmptyDASSector( Word36* const pDAS,
                                                     const LDATINDEX    ldatIndex );
    static void                 calculateCycleInfo( Word36* const       pLeadItem0,
                                                    const Word36* const pLeadItem1 );
    static void                 dumpMFDTrack( std::ostream&         stream,
                                                const TRACK_ID      directoryTrackId,
                                                const LDATINDEX     ldatIndex,
                                                const TRACK_ID      deviceTrackId,
                                                const Word36* const pData,
                                                const UINT64        allocMask );
    static void                 getCommonFileCycleInfo( FileCycleInfo* const    pInfo,
                                                        const DSADDR            mainItem0Addr,
                                                        const Word36* const     pMainItem0,
                                                        const DSADDR            mainItem1Addr,
                                                        const Word36* const     pMainItem1 );
    static void                 getMBTOffsetAndMask( const TRACK_ID     trackId,
                                                     COUNT32* const     pWordOffset,
                                                     UINT64* const      pBitMask );
    static void                 internalPrepBuildMBT( Word36* const         pMasterBitTable,
                                                      const TRACK_COUNT     totalTracks,
                                                      const TRACK_COUNT     reservedTracks,
                                                      const TRACK_COUNT     directoryTracks,
                                                      const SECTOR_ID       firstDirectoryTrack,
                                                      const WORD_COUNT      mbtWords );
    static void                 internalPrepBuildSector0( Word36* const     pSector0,
                                                          const SECTOR_ID   firstDirectoryTrackAddress,
                                                          const TRACK_COUNT directoryTracks,
                                                          const WORD_COUNT  hmbtLength,
                                                          const WORD_COUNT  smbtLength );
    static void                 internalPrepBuildSector1( Word36* const         pSector1,
                                                          const SECTOR_ID       HMBTdeviceSectorAddress,
                                                          const SECTOR_ID       SMBTdeviceSectorAddress,
                                                          const TRACK_COUNT     maxAvailableTracks,     //  not counting initial reserve
                                                          const std::string&    packName,
                                                          const WORD_COUNT      mbtLengthInWords,
                                                          const SECTOR_COUNT    dasOffset );

    //  private inline static
    inline static DSADDR        getLinkAddress( const Word36& word36 )        { return word36.getW() & 07777777777ll; }


public:
    //  constructors, destructors
    MFDManager( Exec* const pExec );
    ~MFDManager(){};

    //  public methods
    Result                      allocateFileTracks( Activity* const         pActivity,
                                                    DiskFacilityItem* const pDiskItem,
                                                    const TRACK_ID          fileTrackId,
                                                    const TRACK_COUNT       trackCount );
    //TODO:ER need an allocateTracks() for EACQ$
    Result                      assignFileCycle( Activity* const                pActivity,
                                                 const DSADDR                   mainItem0Addr,
                                                 const bool                     exclusiveFlag,
                                                 const COUNT32                  newInitialReserve,
                                                 const COUNT32                  newMaxGranules,
                                                 FileAllocationTable** const    ppFileAllocationTable );
    Result                      bringPackOnline( Activity* const                pActivity,
                                                 const DeviceManager::DEVICE_ID deviceId );
    Result                      createFileCycle( Activity* const        pActivity,
                                                 const DSADDR           leadItem0Addr,
                                                 const std::string&     accountId,
                                                 const bool             saveOnCheckpoint,
                                                 const bool             storeThrough,
                                                 const bool             positionGranularity,
                                                 const bool             wordAddressable,
                                                 const std::string&     assignMnemonic,
                                                 const bool             unloadInhibited,
                                                 const bool             privateFile,
                                                 const bool             readInhibited,
                                                 const bool             writeInhibited,
                                                 const UINT16           absoluteCycle,
                                                 const COUNT32          initialReserve,
                                                 const COUNT32          maxGranules,
                                                 const bool             toBeCataloged );
    Result                      createFileSet( Activity* const          pActivity,
                                               const std::string&       qualifier,
                                               const std::string&       filename,
                                               const std::string&       projectId,
                                               const std::string&       readKey,
                                               const std::string&       writeKey,
                                               const FileType           fileType,
                                               const bool               guardedFile,
                                               DSADDR* const            pDSAddr );
    Result                      dropFileCycle( Activity* const  pActivity,
                                               const DSADDR     mainItem0Addr,
                                               const bool       commitFlag = true );
    Result                      dropFileSet( Activity* const    pActivity,
                                             const DSADDR       leadItem0Addr );
    void                        getAssignedNodeIds( DeviceManager::NODE_IDS* const pContainer ) const;
    bool                        getDeviceId( const LDATINDEX                    ldatIndex,
                                             DeviceManager::DEVICE_ID* const    pDeviceId ) const;
    Result                      getDirectorySector( const DSADDR        directorySectorAddress,
                                                    Word36* const       pBuffer ) const;
    Result                      getFileCycleInfo( const DSADDR          mainItem0Addr,
                                                  FileCycleInfo* const  pFileCycleInfo );
    Result                      getFileSetInfo( const std::string&  qualifier,
                                                const std::string&  filename,
                                                FileSetInfo* const  pFileSetInfo );
    Result                      getFileSetInfoList( FileSetInfoList* const  pList );
    Result                      getPackTrackCounts( const PackInfo* const   pPackInfo,
                                                    TRACK_COUNT* const      pAccessible,
                                                    TRACK_COUNT* const      pAvailable ) const;
    Result                      getFixedPoolTrackCounts( TRACK_COUNT* const pAccessible,
                                                         TRACK_COUNT* const pAvailable ) const;
    Result                      getMassStorageFileCycleInfo( const DSADDR                       mainItem0Addr,
                                                             MassStorageFileCycleInfo* const    pFileCycleInfo );
    const PackInfo*             getPackInfo( const DeviceManager::DEVICE_ID deviceId ) const;
    bool                        getPrepFactor( const LDATINDEX      ldatIndex,
                                               PREP_FACTOR* const   pPrepFactor ) const;
    Result                      getSearchItemAddresses( const SuperString&  qualifier,
                                                        const SuperString&  filename,
                                                        DSADDRLIST*         pAddresses ) const;
    Result                      getTapeFileCycleInfo( const DSADDR              mainItem0Addr,
                                                      TapeFileCycleInfo* const  pFileCycleInfo );
    Result                      initialize( Activity* const pActivity );
    Result                      prepDisk( Activity* const               pActivity,
                                          const DeviceManager::NODE_ID  nodeId,
                                          const bool                    fixedFlag,
                                          const SuperString&            packLabel );
    Result                      readDiskLabels( Activity* const pActivity,
                                                COUNT* const    pFixedPacks );
#if 0   //TODO:RECOV
    Result                      recover();
#endif
    Result                      releaseExclusiveUse( Activity* const    pActivity,
                                                     const DSADDR       mainItem0Addr );
    Result                      releaseFileCycle( Activity* const   pActivity,
                                                  const DSADDR      mainItem0Addr );
    Result                      releaseFileTracks( Activity* const          pActivity,
                                                   DiskFacilityItem* const  pDiskItem,
                                                   const TRACK_ID           fileTrackId,
                                                   const TRACK_COUNT        trackCount );
    Result                      setBadTrack( Activity* const   pActivity,
                                             const LDATINDEX   ldatIndex,
                                             const TRACK_ID    trackId );
    Result                      updateFileCycle( Activity* const    pActivity,
                                                 const DSADDR       mainItem0Addr,
                                                 const bool         exclusiveUseFlag,
                                                 const COUNT32      initialReserve,
                                                 const COUNT32      maxGranules );

    //  public statics
    static FileType             getFileType( const UINT8 mfdFileType );
    static std::string          getFileTypeString( const FileType fileType );
    static std::string          getResultString( const Result& result );
    static std::string          getStatusString( const Status status );

    //  Exec::ExecManager interface
    void                        cleanup();
    void                        dump( std::ostream&     stream,
                                      const DUMPBITS    dumpBits );
    void                        shutdown();
    bool                        startup();
    void                        terminate();
};



#endif
