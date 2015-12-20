//  DiskFacilityItem.h
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EXECLIB_DISK_FACILITY_ITEM_H
#define     EXECLIB_DISK_FACILITY_ITEM_H



#include    "FileAllocationTable.h"
#include    "StandardFacilityItem.h"



class DiskFacilityItem : public StandardFacilityItem
{
private:
    FileAllocationTable* const      m_pFileAllocationTable; //  Pointer to FileAllocationTable
                                                            //      For temporary files, we own this
                                                            //      For cataloged files, MFD owns this
    const MassStorageGranularity    m_Granularity;
    UINT32                          m_HighestGranuleAssigned;
    UINT32                          m_HighestTrackWritten;
    UINT32                          m_InitialGranules;
    UINT32                          m_MaximumGranules;

public:
    DiskFacilityItem( //    FacilityItem parameters
                        const std::string&                      fileName,
                        const std::string&                      qualifier,
                        const EquipmentCode                     equipmentCode,
                        const UINT32                            assignOptions,
                        const bool                              releaseFlag,
                        //  StandardFacilityItem parameters
                        const UINT16                            absoluteFileCycle,
                        const bool                              absoluteFileCycleExists,
                        const bool                              exclusiveFlag,
                        const bool                              existingFileFlag,
                        const DSADDR                            mainItem0Addr,
                        const bool                              readInhibitedFlag,
                        const bool                              readKeyNeededFlag,
                        const INT8                              relativeFileCycle,
                        const bool                              relativeFileCycleSpecified,
                        const bool                              temporaryFileFlag,
                        const bool                              writeInhibitedFlag,
                        const bool                              writeKeyNeededFlag,
                        //  DiskFacilityItem parameters
                        FileAllocationTable* const              pFileAllocationTable,
                        const MassStorageGranularity            granularity,
                        const UINT32                            highestGranuleAssigned,
                        const UINT32                            highestTrackWritten,
                        const UINT32                            initialGranules,
                        const UINT32                            maximumGranules )
        :StandardFacilityItem( fileName,
                                qualifier,
                                equipmentCode,
                                assignOptions,
                                releaseFlag,
                                absoluteFileCycle,
                                absoluteFileCycleExists,
                                exclusiveFlag,
                                existingFileFlag,
                                mainItem0Addr,
                                readInhibitedFlag,
                                readKeyNeededFlag,
                                relativeFileCycle,
                                relativeFileCycleSpecified,
                                temporaryFileFlag,
                                writeInhibitedFlag,
                                writeKeyNeededFlag ),
        m_pFileAllocationTable( pFileAllocationTable ),
        m_Granularity( granularity ),
        m_HighestGranuleAssigned( highestGranuleAssigned ),
        m_HighestTrackWritten( highestTrackWritten ),
        m_InitialGranules( initialGranules ),
        m_MaximumGranules( maximumGranules )
    {}

    ~DiskFacilityItem()
    {
        if ( getTemporaryFileFlag() )
            delete m_pFileAllocationTable;
    }

    virtual void    dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const IDENTIFIER    identifier ) const;
    bool            isSectorMassStorage() const                         { return m_EquipmentCode == ECODE_SECTOR_DISK; }
    bool            isWordMassStorage() const                           { return m_EquipmentCode == ECODE_WORD_DISK; }

    inline FileAllocationTable*     getFileAllocationTable() const      { return m_pFileAllocationTable; }
    inline MassStorageGranularity   getGranularity() const              { return m_Granularity; }
    inline UINT32                   getHighestGranuleAssigned() const   { return m_HighestGranuleAssigned; }
    inline UINT32                   getHighestTrackWritten() const      { return m_HighestTrackWritten; }
    inline UINT32                   getInitialGranules() const          { return m_InitialGranules; }
    inline UINT32                   getMaximumGranules() const          { return m_MaximumGranules; }

    inline void     setHighestGranuleAssigned( const UINT32 value )     { m_HighestGranuleAssigned = value; }
    inline void     setHighestTrackWritten( const UINT32 value )        { m_HighestTrackWritten = value; }
    inline void     setInitialGranules( const UINT32 value )            { m_InitialGranules = value; }
    inline void     setMaximumGranules( const UINT32 value )            { m_MaximumGranules = value; }
};



#endif

