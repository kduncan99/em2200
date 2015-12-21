//  Implementation of SubSystem class
//  Copyright (c) 2015 by Kurt Duncan



#include    "emssp.h"



//  Public methods

//  removeDevice()
//
//  Detaches a device from all controllers and removes it from the subsystem
void
SubSystem::removeDevice
(
    Device* const       pDevice
)
{
    for ( auto itctl = m_Controllers.begin(); itctl != m_Controllers.end(); ++itctl )
    {
        Controller* pctl = *itctl;
        Node::disconnect( pctl, pDevice );
    }

    for ( auto itdev = m_Devices.begin(); itdev != m_Devices.end(); ++itdev )
    {
        if ( *itdev == pDevice )
        {
            m_Devices.erase( itdev );
            break;
        }
    }
}

