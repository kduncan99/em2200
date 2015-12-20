//  DiskFacilityItem implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  dump()
void
    DiskFacilityItem::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const IDENTIFIER        identifier
    ) const
{
    stream << prefix << "Disk Facility Item";
    StandardFacilityItem::dump( stream, prefix, identifier );

    stream << prefix << "  Granularity:              " << execGetGranularityString( m_Granularity ) << std::endl;
    stream << prefix << "  Highest Granule Assigned: 0" << std::oct << m_HighestGranuleAssigned << std::endl;
    stream << prefix << "  Highest Track Written:    0" << std::oct << m_HighestTrackWritten << std::endl;
    stream << prefix << "  Initial Granules:         0" << std::oct << m_InitialGranules << std::endl;
    stream << prefix << "  Max Granules:             0" << std::oct << m_MaximumGranules << std::endl;
}


