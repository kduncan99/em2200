//	SUKeyin.cpp
//
//	Handles the SU keyin



#include	"execlib.h"



//	private / protected methods

//	handler()
//
//	Called from Keyin base class worker() function
void
    SUKeyin::handler()
{
    //  We expect exactly one parameter
    if ( m_Parameters.size() != 1 )
    {
        displayGeneralError();
        return;
    }

    NODEENTRYSET nodeEntries;
    const DeviceManager::NodeEntry* pKeyinNodeEntry = 0;    //  The NodeEntry for the component on the keyin

    //  If there is an option, it must be 'ALL', and the parameter must be a disk controller
	if ( m_Option.size() > 0 )
    {
        if ( m_Option.compareNoCase( "ALL" ) != 0 )
        {
            displayGeneralError();
            return;
        }

        pKeyinNodeEntry = m_pDeviceManager->getNodeEntry( m_Parameters[0] );
        if ( pKeyinNodeEntry == 0 )
        {
            notifyDoesNotExist( m_Parameters[0] );
            return;
        }

        Node* pNode = pKeyinNodeEntry->m_pNode;
        if ( !pKeyinNodeEntry->isController()
            || ( reinterpret_cast<const Controller*>(pNode)->getControllerType() != Controller::ControllerType::DISK ) )
        {
            notifyKeyinNotAllowed( pKeyinNodeEntry );
            return;
        }

        for ( DeviceManager::CITNODEIDMAP itni = pKeyinNodeEntry->m_ChildNodeIds.begin();
            itni != pKeyinNodeEntry->m_ChildNodeIds.end(); ++itni )
        {
            const DeviceManager::NodeEntry* pChildNodeEntry = m_pDeviceManager->getNodeEntry( itni->second );
            if ( pChildNodeEntry == 0 )
            {
                std::stringstream strm;
                strm << "SUKeyin::Could not resolve nodeId "
                    << itni->second
                    << " in child node list for node entry for "
                    << pKeyinNodeEntry->m_pNode->getName();
                SystemLog::write( strm.str() );
                m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
                return;
            }

            nodeEntries.insert( pChildNodeEntry );
        }
    }
    else
    {
        //  The parameter must be a disk device
        pKeyinNodeEntry = m_pDeviceManager->getNodeEntry( m_Parameters[0] );
        if ( pKeyinNodeEntry == 0 )
        {
            notifyDoesNotExist( m_Parameters[0] );
            return;
        }

        Node* pNode = pKeyinNodeEntry->m_pNode;
        if ( !pKeyinNodeEntry->isDevice()
            || ( reinterpret_cast<const Device*>(pNode)->getDeviceType() != Device::DeviceType::DISK ) )
        {
            notifyKeyinNotAllowed( pKeyinNodeEntry );
            return;
        }

        nodeEntries.insert( pKeyinNodeEntry );
    }

    //  Is mass storage very tight?  If so, ask operator if he really wants to do this.
    //  Documentation mentions MSW5 configuration parameter (or some parameter).
    //  For now, I'll just make it 50 tracks.  But it should be tied to the MS Tight values...
    /*????
    The MASS STORAGE TIGHT condition occurs when the availability of standard
    fixed mass storage falls below 12.5 percent of the STDMSTRT configuration
    parameter.
    */
    if ( m_pExec->getStatus() == Exec::ST_RUNNING )
    {
        TRACK_COUNT accessible = 0;
        TRACK_COUNT available = 0;
        m_pMFDManager->getFixedPoolTrackCounts( &accessible, &available );
        if ( available < 50 )
        {
            std::string msg = "SU " + m_Parameters[0] + " MAY BE FATAL (EXERR 052) - TERMINATE SU KEYIN? Y/N";
            PollResult pollResult = pollOperator( msg );
            if ( pollResult == PR_YES )
            {
                notifyKeyinAborted( pKeyinNodeEntry );
                return;
            }
            else if ( pollResult == PR_CANCELED )
                return;
        }
    }

    //  Suspend the disk device(s)
    for ( CITNODEENTRYSET itn = nodeEntries.begin(); itn != nodeEntries.end(); ++itn )
    {
        //  Is this device already suspended?  If so, notify operator and move on.
        //  Otherwise, pass it along to DeviceManager...
        if ( (*itn)->m_Status == DeviceManager::NDST_SU )
            notifyKeyinAlreadyPerformed( *itn );
        else
            m_pDeviceManager->setNodeStatus( this, (*itn)->m_NodeId, DeviceManager::NDST_SU );
    }

    //  Echo result to console
    generateOutput( nodeEntries );
}


//  isAllowed()
bool
    SUKeyin::isAllowed() const
{
    Exec::Status execStatus = m_pExec->getStatus();
    return (execStatus == Exec::ST_BOOTING_1) || (execStatus == Exec::ST_RUNNING);
}



// constructors / destructors

SUKeyin::SUKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:FacilitiesKeyin( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods

