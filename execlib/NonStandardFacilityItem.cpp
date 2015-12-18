//  NonStandardFacilityItem implementation



#include    "execlib.h"



//  dump()
void
    NonStandardFacilityItem::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const IDENTIFIER        identifier
    ) const
{
    stream << "NonStandard Facility Item";
    FacilityItem::dump( stream, prefix, identifier );
}


