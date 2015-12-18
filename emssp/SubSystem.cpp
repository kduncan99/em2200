//  Implementation of SubSystem class



#include    "emssp.h"



//  Public methods

void
SubSystem::removeDevice
(
    Device* const       pDevice
)
{
    for ( auto itctl = m_Controllers.begin(); itctl != m_Controllers.end(); ++itctl )
    {
        Controller* pctl = *itctl;
        for ( auto itchild = pctl->getChildNodes().begin(); itchild != pctl->getChildNodes().end(); ++itchild )
        {
            if ( itchild->second == pDevice )
            {
                pctl->deregisterChildNode( itchild->first );
                //  we can stop on the first find, since we only allow one connection
                //  from a ctl to a device.
                break;
            }
        }
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

