//  NonStandardFacilityItem.h
//
//  Refers to absolute device and arbitrary device assignments.
//  TODO:TAPE - do we use this for tape devices even if we have a standard item for the tape file?



#ifndef     EXECLIB_NON_STANDARD_FACILITY_ITEM_H
#define     EXECLIB_NON_STANDARD_FACILITY_ITEM_H



#include    "FacilityItem.h"



class NonStandardFacilityItem : public FacilityItem
{
public:
    NonStandardFacilityItem( const std::string&     fileName,
                                const std::string&  qualifier,
                                const EquipmentCode equipmentCode,
                                const UINT32        assignOptions,
                                const bool          releaseFlag )
        :FacilityItem( fileName, qualifier, equipmentCode, assignOptions, releaseFlag )
    {}

    ~NonStandardFacilityItem(){}

    void            dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const IDENTIFIER    identifier ) const;
    bool            isNonStandard() const           { return true; }
};



#endif

