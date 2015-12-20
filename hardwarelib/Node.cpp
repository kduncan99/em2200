//  Node class implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "hardwarelib.h"



//  publics

void
Node::dump
(
std::ostream&      stream
) const
{
    stream << "Node " << getName()
        << "  Category:" << getCategoryString( getCategory() ) << std::endl;
    stream << "  Child Nodes:";
    for ( CITCHILDNODES itn = m_ChildNodes.begin(); itn != m_ChildNodes.end(); ++itn )
        stream << "    [" << itn->first << "] " << itn->second->getName();
}


//  static publics

const char*
Node::getCategoryString
    (
    const Category  category
    )
{
    switch ( category )
    {
    case Category::PROCESSOR:       return "Processor";
    case Category::CHANNEL_MODULE:  return "ChannelModule";
    case Category::CONTROLLER:      return "Controller";
    case Category::DEVICE:          return "Device";
    }

    return "?";
}

