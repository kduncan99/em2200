//  Processor.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Processor class definition



//  includes

#include    "hardwarelib.h"



// private / protected functions


// constructors / destructors


//  public functions

//  dump()
void
Processor::dump
(
std::ostream&       stream
) const
{
    Node::dump( stream );
    stream << "  Type: " << getProcessorTypeString( m_ProcessorType ) << std::endl;
}



//  public static functions

//  getProcessorTypeString())
//
//  Converts a ProcessorType value to a displayable string
//
//  Parameters:
//      type:               value to be converted
//
//  Returns:
//      string containing displayable text
const char*
Processor::getProcessorTypeString
(
const Processor::ProcessorType        type
)
{
    switch ( type )
    {
    case ProcessorType::IP:         return "IP";
    case ProcessorType::IOP:        return "IOP";
    }

    return "?";
}

