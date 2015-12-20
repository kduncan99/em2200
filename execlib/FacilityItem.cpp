//  FacilityItem implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  dump()
//
//  Dumps common items to the given stream.
//  To be called by dump() from the descendent class, after that class sends the item type to stream
//  (with no endline - our first output appends to that bit of information)
void
    FacilityItem::dump
    (
    std::ostream&           stream,
    const std::string&      prefix,
    const IDENTIFIER        identifier
    ) const
{
    stream << "  Identifier=0" << std::oct << identifier << " " << m_Qualifier << "*" << m_FileName << std::endl;
    stream << prefix << "  Equip Code:         " << std::oct << m_EquipmentCode
        << " (" << getEquipmentCodeString( m_EquipmentCode ) << ")" << std::endl;
    stream << prefix << "  AsgOptions:         " << execGetOptionsString( m_AssignOptions ) << std::endl;
    stream << prefix << "  Release at Task End:" << ( m_ReleaseFlag ? "YES" : "NO" ) << std::endl;
}



//  public statics

std::string
    FacilityItem::getEquipmentCodeString
    (
    const EquipmentCode         code
    )
{
    switch ( code )
    {
    case ECODE_NONE:           return "None";
    case ECODE_UNISERVO_7:     return "Uniservo 22D/24D/30D";
    case ECODE_UNISERVO_9:     return "Uniservo 26N/28N/32N/34N/36N/45N";
    case ECODE_VTH:            return "Virtual Tape Handler";
    case ECODE_CTAPE:          return "CT0899/SCTAPE/U47/U47L/U47M/U47LM/U5136/U5236/DLT7000/DVDTP";
    case ECODE_WORD_DISK:      return "Word-addressable Mass Storage";
    case ECODE_SECTOR_DISK:    return "Sector-addressable Mass Storage";
    case ECODE_CHANNEL:        return "HPRDEV/DCPBDV/HLCDEV";
    case ECODE_ARBDEV:         return "ARBDEV/AC40/CRYPDV/CTLDEV";
    }

    std::stringstream strm;
    strm << "0" << std::oct << code;
    return strm.str();
}

