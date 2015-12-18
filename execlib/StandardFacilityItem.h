//  StandardFacilityItem.h
//
//  Refers (generally) to either cataloged or temporary disk or file



#ifndef     EXECLIB_STANDARD_FACILITY_ITEM_H
#define     EXECLIB_STANDARD_FACILITY_ITEM_H



#include    "FacilityItem.h"



class StandardFacilityItem : public FacilityItem
{
protected:
    const UINT16                    m_AbsoluteFileCycle;        //  actual absolute file cycle
    const bool                      m_AbsoluteFileCycleExists;  //  true if user specified it, false if not (meaningful only for temp files)
    const bool                      m_CatalogOnAnyTermFlag;
    const bool                      m_CatalogOnNormalTermFlag;
    const bool                      m_DeleteOnAnyTermFlag;      //  Only for non-temporary files
    const bool                      m_DeleteOnNormalTermFlag;   //  Only for non-temporary files
    bool                            m_ExclusiveFlag;            //  File assigned exclusively to this run
    const bool                      m_ExistingFileFlag;         //  File already exists
    const DSADDR                    m_MainItem0Addr;            //  Only for cataloged files
    const bool                      m_ReadInhibitedFlag;
    const bool                      m_ReadKeyNeededFlag;
    INT8                            m_RelativeFileCycle;
    bool                            m_RelativeFileCycleSpecified;
    const bool                      m_TemporaryFileFlag;        //  Temporary file
    const bool                      m_WriteInhibitedFlag;
    const bool                      m_WriteKeyNeededFlag;

public:
    StandardFacilityItem(   //  FacilityItem
                            const std::string&                  fileName,
                            const std::string&                  qualifier,
                            const EquipmentCode                 equipmentCode,
                            const UINT32                        assignOptions,
                            const bool                          releaseFlag,
                            //  StandardFacilityItem
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
                            const bool                          writeKeyNeededFlag )
        :FacilityItem( fileName, qualifier, equipmentCode, assignOptions, releaseFlag ),
        m_AbsoluteFileCycle( absoluteFileCycle ),
        m_AbsoluteFileCycleExists( absoluteFileCycleExists ),
        m_CatalogOnAnyTermFlag( (assignOptions & OPTB_U) != 0 ),
        m_CatalogOnNormalTermFlag( (assignOptions & OPTB_C) != 0 ),
        m_DeleteOnAnyTermFlag( (assignOptions & OPTB_K) != 0 ),
        m_DeleteOnNormalTermFlag( (assignOptions & OPTB_D) != 0 ),
        m_ExclusiveFlag( exclusiveFlag ),
        m_ExistingFileFlag( existingFileFlag ),
        m_MainItem0Addr( mainItem0Addr ),
        m_ReadInhibitedFlag( readInhibitedFlag ),
        m_ReadKeyNeededFlag( readKeyNeededFlag ),
        m_RelativeFileCycle( relativeFileCycle ),
        m_RelativeFileCycleSpecified( relativeFileCycleSpecified ),
        m_TemporaryFileFlag( temporaryFileFlag ),
        m_WriteInhibitedFlag( writeInhibitedFlag ),
        m_WriteKeyNeededFlag( writeKeyNeededFlag )
    {}

    ~StandardFacilityItem(){}

    virtual void    dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const IDENTIFIER    identifier ) const;

    inline UINT16   getAbsoluteFileCycle() const                        { return m_AbsoluteFileCycle; }
    inline bool     getAbsoluteFileCycleExists() const                  { return m_AbsoluteFileCycleExists; }
    inline bool     getCatalogOnAnyTermFlag() const                     { return m_CatalogOnAnyTermFlag; }
    inline bool     getCatalogOnNormalTermFlag() const                  { return m_CatalogOnNormalTermFlag; }
    inline bool     getDeleteOnAnyTermFlag() const                      { return m_DeleteOnAnyTermFlag; }
    inline bool     getDeleteOnNormalTermFlag() const                   { return m_DeleteOnNormalTermFlag; }
    inline bool     getExclusiveFlag() const                            { return m_ExclusiveFlag; }
    inline bool     getExistingFileFlag() const                         { return m_ExistingFileFlag; }
    inline DSADDR   getMainItem0Addr() const                            { return m_MainItem0Addr; }
    inline bool     getReadInhibitedFlag() const                        { return m_ReadInhibitedFlag; }
    inline bool     getReadKeyNeededFlag() const                        { return m_ReadKeyNeededFlag; }
    inline INT8     getRelativeFileCycle() const                        { return m_RelativeFileCycle; }
    inline bool     getRelativeFileCycleSpecified() const               { return m_RelativeFileCycleSpecified; }
    inline bool     getTemporaryFileFlag() const                        { return m_TemporaryFileFlag; }
    inline bool     getWriteInhibitedFlag() const                       { return m_WriteInhibitedFlag; }
    inline bool     getWriteKeyNeededFlag() const                       { return m_WriteKeyNeededFlag; }
    inline void     setExclusiveFlag( const bool flag )                 { m_ExclusiveFlag = flag; }
    inline void     setRelativeFileCycle( const INT8 value )            { m_RelativeFileCycle = value; }
    inline void     setRelativeFileCycleSpecified( const bool flag )    { m_RelativeFileCycleSpecified = flag; }
    inline void     setReleaseFlag( const bool flag )                   { m_ReleaseFlag = flag; }
};



#endif

