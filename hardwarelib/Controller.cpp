//  Controller implementation
//  Copyright (c) 2015 by Kurt Duncan
//
//  Not much here



#include    "hardwarelib.h"



//  private, protected methods



//  constructors, destructors



//  public methods

//  routeIo()
//
//  Routes an IO described by the given IoInfo object, to the child identified by the address
void
Controller::routeIo
(
    const NODE_ADDRESS          address,
    Device::IoInfo* const       pIoInfo
)
{
    ITCHILDNODES itd = m_ChildNodes.find(address);
    if ( itd == m_ChildNodes.end() )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_DEVICE_ADDRESS );
        pIoInfo->getSource()->signal( this );
    }
    else
    {
        pIoInfo->setStatus( Device::IoStatus::IN_PROGRESS );
        Device* pd = dynamic_cast<Device*>( itd->second );
        pd->handleIo( pIoInfo );
    }
}



//  public static methods

//  getControllerTypeString())
//
//  Converts a ControllerType value to a displayable string
//
//  Parameters:
//      type:               value to be converted
//
//  Returns:
//      string containing displayable text
const char*
Controller::getControllerTypeString
(
const Controller::ControllerType        type
)
{
    switch ( type )
    {
    case ControllerType::DISK:      return "Disk";
    case ControllerType::SYMBIONT:  return "Symbiont";
    case ControllerType::TAPE:      return "Tape";
    }

    return "?";
}

