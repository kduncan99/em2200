//  StandardFacilityItem implementation



#include    "execlib.h"



//  dump()
//
//  Descendent class dump code calls here after emitting type-specific identifier to stream
void
    StandardFacilityItem::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const IDENTIFIER        identifier
    ) const
{
    FacilityItem::dump( stream, prefix, identifier );

    //  Now dump our own stuff to stream
    stream << prefix << "  Absolute Cycle:     ";
    if ( m_AbsoluteFileCycleExists )
        stream << "Exists=" << m_AbsoluteFileCycle << std::endl;
    else
        stream << "Does not exist" << std::endl;

    stream << prefix << "  Relative Cycle:     ";
    if ( m_RelativeFileCycleSpecified )
        stream << "Specified=" << m_RelativeFileCycle << std::endl;
    else
        stream << "Does not exist" << std::endl;

    stream << prefix << "  Exclusive:          " << (m_ExclusiveFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Delete On Any Term: " << (m_DeleteOnAnyTermFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Delete On Nrm Term: " << (m_DeleteOnNormalTermFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Read Key Needed:    " << (m_ReadKeyNeededFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Write Key Needed:   " << (m_WriteKeyNeededFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Read Inhibited:     " << (m_ReadInhibitedFlag ? "Yes" : "No") << std::endl;
    stream << prefix << "  Write Inhibited:    " << (m_WriteInhibitedFlag ? "Yes" : "No") << std::endl;
}


