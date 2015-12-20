//  FacilityItem.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Represents a fac item in the RunInfo object



#ifndef     EXECLIB_FACILITY_ITEM_H
#define     EXECLIB_FACILITY_ITEM_H



class FacilityItem
{
public:
    typedef UINT32              IDENTIFIER;
    static const IDENTIFIER     INVALID_IDENTIFIER = 0xFFFFFFFF;

protected:
    const UINT32                m_AssignOptions;        //  Original @ASG opts: A is bit 25, Z is bit 0(LSB); Bits 31(MSB)-26 are zero
    const EquipmentCode         m_EquipmentCode;        //  Based on equipment type
    const SuperString           m_FileName;
    IDENTIFIER                  m_Identifier;           //  Uniquely ID's this item -- cannot be provided at creation time,
                                                        //      as it only exists once we're appended to a RunInfo object.
    const SuperString           m_Qualifier;
    bool                        m_ReleaseFlag;          //  Release fac item at next task termination

    FacilityItem( const std::string&    fileName,
                    const std::string&  qualifier,
                    const EquipmentCode equipmentCode,
                    const UINT32        assignOptions,
                    const bool          releaseFlag )
        :m_AssignOptions( assignOptions ),
        m_EquipmentCode( equipmentCode ),
        m_FileName( fileName ),
        m_Identifier( INVALID_IDENTIFIER ),
        m_Qualifier( qualifier ),
        m_ReleaseFlag( releaseFlag )
    {}

public:
    virtual ~FacilityItem(){}

    inline UINT32               getAssignOptions() const                { return m_AssignOptions; }
    inline EquipmentCode        getEquipmentCode() const                { return m_EquipmentCode; }
    inline const SuperString&   getFileName() const                     { return m_FileName; }
    inline IDENTIFIER           getIdentifier() const                   { return m_Identifier; }
    inline const SuperString&   getQualifier() const                    { return m_Qualifier; }
    inline bool                 getReleaseFlag() const                  { return m_ReleaseFlag; }
    void                        setIdentifier( const IDENTIFIER id )    { m_Identifier = id; }

    virtual void        dump( std::ostream&         stream,
                                const std::string&  prefix,
                                const IDENTIFIER    identifier ) const;
    virtual bool        isNonStandard() const                   { return false; }
    virtual bool        isSectorMassStorage() const             { return false; }
    virtual bool        isTape() const                          { return false; }
    virtual bool        isWordMassStorage() const               { return false; }

    static std::string  getEquipmentCodeString( const EquipmentCode code );
};


typedef     std::map<FacilityItem::IDENTIFIER, FacilityItem*>   FACITEMS;
typedef     FACITEMS::iterator                                  ITFACITEMS;
typedef     FACITEMS::const_iterator                            CITFACITEMS;



#endif

