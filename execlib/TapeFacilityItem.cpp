//  TapeFacilityItem implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  dump()
void
    TapeFacilityItem::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const IDENTIFIER        identifier
    ) const
{
    stream << "Tape Facility Item";
    StandardFacilityItem::dump( stream, prefix, identifier );

    //  Do our special stuff here
    stream << prefix << "  Logical Channel:    " << (m_LogicalChannel ? m_LogicalChannel : ' ') << std::endl;
    stream << prefix << "  Noise Constant:     " << m_NoiseConstant << std::endl;
    stream << prefix << "  Expiration Period:  " << m_ExpirationPeriod << std::endl;
    stream << prefix << "  Unit Count:         " << m_UnitCount << std::endl;
    stream << prefix << "  Reel Index:         " << m_ReelIndex << std::endl;
    stream << prefix << "  Current Reel:       " << m_CurrentReelNumber << std::endl;
    stream << prefix << "  Next Reel:          " << m_NextReelNumber << std::endl;
    stream << prefix << "  Total Reel Count:   " << m_TotalReelCount << std::endl;
    stream << prefix << "  Files Extended:     " << m_FilesExtended << std::endl;
    stream << prefix << "  Blocks Extended:    " << m_BlocksExtended << std::endl;
}


