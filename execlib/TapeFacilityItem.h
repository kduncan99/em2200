//  TapeFacilityItem.h



#ifndef     EXECLIB_TAPE_FACILITY_ITEM_H
#define     EXECLIB_TAPE_FACILITY_ITEM_H



#include    "DeviceManager.h"
#include    "StandardFacilityItem.h"



class TapeFacilityItem : public StandardFacilityItem
{
private:
    DeviceManager::NODE_IDS             m_AssignedNodes;
    COUNT                               m_BlocksExtended;
    INDEX                               m_CurrentNodeIndex;
    SuperString                         m_CurrentReelNumber;
    UINT16                              m_ExpirationPeriod;
    COUNT                               m_FilesExtended;
    char                                m_LogicalChannel;
    SuperString                         m_NextReelNumber;
    UINT8                               m_NoiseConstant;
    INDEX                               m_ReelIndex;
    TapeFormat                          m_TapeFormat;
    COUNT                               m_TotalReelCount;
    const COUNT                         m_UnitCount;

public:
    TapeFacilityItem( const std::string&                    fileName,
                        const std::string&                  qualifier,
                        const EquipmentCode                 equipmentCode,
                        const UINT32                        assignOptions,
                        const bool                          releaseFlag,
                        //  base base class above
                        const UINT16                        absoluteFileCycle,
                        const bool                          absoluteFileCycleExists,
                        const bool                          exclusiveFlag,
                        const bool                          existingFileFlag,
                        const DSADDR                        mainItem0Addr,
                        const bool                          readInhibitedFlag,
                        const bool                          readKeyNeededFlag,
                        const INT8                          relativeFileCycle,
                        const bool                          relativeFileCycleSpecified,
                        const bool                          temporaryFileFlag,
                        const bool                          writeInhibitedFlag,
                        const bool                          writeKeyNeededFlag,
                        //  base class above, us below
                        const DeviceManager::NODE_IDS&      assignedNodes,
                        const std::string&                  currentReelNumber,
                        const UINT16                        expirationPeriod,
                        const char                          logicalChannel,
                        const std::string&                  nextReelNumber,
                        const UINT8                         noiseConstant,
                        const TapeFormat                    tapeFormat,
                        const COUNT                         totalReelCount,
                        const COUNT                         unitCount )
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
                m_AssignedNodes( assignedNodes ),
                m_BlocksExtended( 0 ),
                m_CurrentNodeIndex( 0 ),
                m_CurrentReelNumber( currentReelNumber ),
                m_ExpirationPeriod( expirationPeriod ),
                m_FilesExtended( 0 ),
                m_LogicalChannel( logicalChannel ),
                m_NextReelNumber( nextReelNumber ),
                m_NoiseConstant( noiseConstant ),
                m_ReelIndex( 0 ),
                m_TapeFormat( tapeFormat ),
                m_TotalReelCount( totalReelCount ),
                m_UnitCount( unitCount )
    {
        m_CurrentReelNumber.foldToUpperCase();
        m_NextReelNumber.foldToUpperCase();
    }

    ~TapeFacilityItem(){}

    void            dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const IDENTIFIER    identifier ) const;
    bool            isTape() const                      { return true; }

    inline const DeviceManager::NODE_IDS&   getAssignedNodes() const        { return m_AssignedNodes; }
    inline COUNT                            getBlocksExtended() const       { return m_BlocksExtended; }
    inline INDEX                            getCurrentNodeIndex() const     { return m_CurrentNodeIndex; }
    inline const SuperString&               getCurrentReelNumber() const    { return m_CurrentReelNumber; }
    inline UINT16                           getExpirationPeriod() const     { return m_ExpirationPeriod; }
    inline COUNT                            getFilesExtended() const        { return m_FilesExtended; }
    inline char                             getLogicalChannel() const       { return m_LogicalChannel; }
    inline const SuperString&               getNextReelNumber() const       { return m_NextReelNumber; }
    inline UINT8                            getNoiseConstant() const        { return m_NoiseConstant; }
    inline INDEX                            getReelIndex() const            { return m_ReelIndex; }
    inline TapeFormat                       getTapeFormat() const           { return m_TapeFormat; }
    inline COUNT                            getTotalReelCount() const       { return m_TotalReelCount; }
    inline COUNT                            getUnitCount() const            { return m_UnitCount; }
};



#endif

