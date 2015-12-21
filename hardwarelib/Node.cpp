//  Node class implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "hardwarelib.h"
#include "Node.h"



//  publics

//  dump()
//
//  For debugging
void
Node::dump
(
std::ostream&      stream
) const
{
    stream << "Node " << getName()
        << "  Category:" << getCategoryString( getCategory() ) << std::endl;
    stream << "  Ancestors:";
    for ( auto ita = m_Ancestors.begin(); ita != m_Ancestors.end(); ++ita )
        stream << "  " << (*ita)->getName();
    for ( auto itd = m_Descendants.begin(); itd != m_Descendants.end(); ++itd )
        stream << "    [" << itd->first << "] " << itd->second->getName();
    stream << std::endl;
}



//  static publics

//  connect()
//
//  Connects two nodes as ancestor/descendant, choosing a unique address
//  Only one such connection may exist between any two nodes, and only between certain categories.
//
//  This is *not* thread-safe - be careful
bool
Node::connect
(
    Node* const         pAncestor,
    Node* const         pDescendant
)
{
    NODE_ADDRESS addr = 0;
    while ( pAncestor->m_Descendants.find( addr ) != pAncestor->m_Descendants.end() )
        ++addr;
    return connect( pAncestor, addr, pDescendant );
}


//  connect()
//
//  Connects two nodes as ancestor/descendant.
//  Only one such connection may exist between any two nodes, and only between certain categories.
//
//  This is *not* thread-safe - be careful
bool
Node::connect
(
    Node* const         pAncestor,
    const NODE_ADDRESS  ancestorAddress,
    Node* const         pDescendant
)
{
    Category aCat = pAncestor->getCategory();
    Category dCat = pDescendant->getCategory();
    bool okay = false;

    //  Are categories / types compatible?
    if ( (aCat == Category::PROCESSOR)
          && (dynamic_cast<Processor*>(pAncestor)->getProcessorType() == Processor::ProcessorType::IOP)
          && (dCat == Category::CHANNEL_MODULE) )
    {
        okay = true;
    }
    else if ( (aCat == Category::CHANNEL_MODULE) && (dCat == Category::CONTROLLER) )
    {
        okay = true;
    }
    else if ( (aCat == Category::CONTROLLER) && (dCat == Category::DEVICE) )
    {
        Controller::ControllerType aType = dynamic_cast<Controller*>(pAncestor)->getControllerType();
        Device::DeviceType dType = dynamic_cast<Device*>(pDescendant)->getDeviceType();
        if ( (aType == Controller::ControllerType::DISK) && (dType == Device::DeviceType::DISK) )
        {
            okay = true;
        }
        if ( (aType == Controller::ControllerType::SYMBIONT) && (dType == Device::DeviceType::SYMBIONT) )
        {
            okay = true;
        }
        if ( (aType == Controller::ControllerType::TAPE) && (dType == Device::DeviceType::TAPE) )
        {
            okay = true;
        }
    }

    if ( !okay )
        return false;

    //  Is a descendant already connected at the indicated ancestor address?
    if ( pAncestor->m_Descendants.find( ancestorAddress ) != pAncestor->m_Descendants.end() )
        return false;

    //  Is this pair already connected?
    if ( pDescendant->m_Ancestors.find( pAncestor ) != pDescendant->m_Ancestors.end() )
        return false;

    //  Create the two-way link
    pAncestor->m_Descendants[ancestorAddress] = pDescendant;
    pDescendant->m_Ancestors.insert( pAncestor );
    return true;
}


//  disconnect()
//
//  Disconnect an ancestor/descendant connection
bool
Node::disconnect
(
    Node* const    pAncestor,
    Node* const    pDescendant
)
{
    auto ita = pDescendant->m_Ancestors.find( pAncestor );
    if ( ita == pDescendant->m_Ancestors.end() )
        return false;
    pDescendant->m_Ancestors.erase( ita );

    for ( auto itd = pAncestor->m_Descendants.begin(); itd != pAncestor->m_Descendants.end(); ++itd )
    {
        if ( itd->second == pDescendant )
        {
            pAncestor->m_Descendants.erase( itd );
            break;
        }
    }

    return true;
}


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

