//	FSKeyin.cpp
//
//	Handles the FS keyin
//
//  Note that, throughout this code, we presume that once given a pointer to a DeviceManager NodeEntry object,
//  that object remains valid until the end of keyin processing.  This is okay, since a NodeEntry will never
//  be deleted except during a very short period of time during DeviceManager startup.
//  ????do we need to prevent this keyin during that short period of time?



#include	"execlib.h"



//	private / protected methods

//  generateAvailabilityOutput()
//
//  Creates output including MS availability for FS,AVAIL
bool
FSKeyin::generateAvailabilityOutput
(
const DeviceManager::NodeEntry* const   pNodeEntry
) const
{
    bool shortFlag = false;
    std::string statusMsg;
    if ( !generateOutputForEntry( pNodeEntry, &statusMsg, &shortFlag ) )
        return false;

    const MFDManager::PackInfo* pPackInfo = m_pMFDManager->getPackInfo( pNodeEntry->m_NodeId );
    if ( !pPackInfo )
        return false;

    TRACK_COUNT accessible = 0;
    TRACK_COUNT available = 0;
    MFDManager::Result mfdResult = m_pMFDManager->getPackTrackCounts( pPackInfo, &accessible, &available );
    if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
        return false;

    std::stringstream availMsg;
    availMsg << pNodeEntry->m_pNode->getName()
        << " "
        << available
        << " LOCAL TRACKS AVAILABLE";
    m_pConsoleManager->postReadOnlyMessage( statusMsg, m_Routing, m_pExec->getRunInfo() );
    m_pConsoleManager->postReadOnlyMessage( availMsg.str(), m_Routing, m_pExec->getRunInfo() );

    return true;
}


//	handler()
//
//	Called from Keyin base class worker() function
void
FSKeyin::handler()
{
	if ( m_Option.size() == 0 )
    {
        if ( (m_Parameters.size() == 1) && (m_Parameters[0].find( '/' ) != m_Parameters[0].npos) )
            handleInterface();
        else
            handleComponent();
    }
    else
    {
        std::string errmsg;
        if ( m_Option.compareNoCase( "ALL" ) == 0 )
            handleALL();
        else if ( m_Option.compareNoCase( "AVAIL" ) == 0 )
            handleAVAIL();
        else if ( m_Option.compareNoCase( "ADISK" ) == 0 )
            handleADISK();
        else if ( m_Option.compareNoCase( "ATAPES" ) == 0 )
            errmsg = "tapes are not yet implemented";       //  TODO:TAPE
        else if ( m_Option.compareNoCase( "CM" ) == 0 )
            handleCM();
        else if ( m_Option.compareNoCase( "DRS" ) == 0 )
            errmsg = "DRS is not used by emexec";
        else if ( m_Option.compareNoCase( "IOP" ) == 0 )
            handleIOP();
        else if ( m_Option.compareNoCase( "IP" ) == 0 )
            errmsg = "emexec does not directly emulate instruction processors";
        else if ( m_Option.compareNoCase( "MEM" ) == 0 )
            errmsg = "emexec does not emulate main storage components";
        else if ( m_Option.compareNoCase( "MS" ) == 0 )
            handleMS();
        else if ( m_Option.compareNoCase( "P" ) == 0 )
            errmsg = "tapes are not yet implemented";       //  TODO:TAPE
        else if ( m_Option.compareNoCase( "PACK" ) == 0 )
            handlePACK();
        else if ( m_Option.compareNoCase( "RDISKS" ) == 0 )
            errmsg = "REM DISK not yet implemented";        //  TODO:REM
        else if ( m_Option.compareNoCase( "SHRD" ) == 0 )
            errmsg = "emexec does not implement SHARED mass storage";
        else if ( m_Option.compareNoCase( "TAPES" ) == 0 )
            errmsg = "tapes are not yet implemented";       //  TODO:TAPE
        else if ( m_Option.compareNoCase( "UD" ) == 0 )
            errmsg = "emexec does not implement Unit Duplexing";
        else if ( m_Option.compareNoCase( "XIIP" ) == 0 )
            errmsg = "emexec does not emulate extended I/O IOPs";
        else if ( m_Option.compareNoCase( "XPC" ) == 0 )
            errmsg = "emexec does not implement extended processing complexes";
        else if ( m_Option.compareNoCase( "XCACHE" ) == 0 )
            errmsg = "emexec does not implement XPC caching";
        else
            errmsg = "FS KEYIN - " + m_Option + " OPTION DOES NOT EXIST, INPUT IGNORED";

        if ( errmsg.size() > 0 )
            m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
    }
}


//  handleALL()
//
//  Handles FS,ALL
void
FSKeyin::handleALL() const
{
    if ( m_Parameters.size() != 1 )
    {
        displayGeneralError();
        return;
    }

    const DeviceManager::NodeEntry* pParent = m_pDeviceManager->getNodeEntry( m_Parameters[0] );
    if ( pParent == 0 )
    {
        notifyDoesNotExist( m_Parameters[0] );
        return;
    }

    NODEENTRYSET nodes;
    if ( !getDownstreamNodes( pParent, &nodes ) )
    {
        m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
        return;
    }

    if ( !nodes.empty() )
        generateOutput( nodes );
}


//  handleADISK()
//
//  Handles FS,ADISK
//  Displays stats for all disk in UP, SU, or RV states
void
FSKeyin::handleADISK() const
{
    if ( m_Parameters.size() > 0 )
    {
        displayGeneralError();
        return;
    }

    //  Retrieve the set of node IDs for all the device nodes
    DeviceManager::NODE_IDS nodeIds;
    m_pDeviceManager->getDeviceIdentifiers( &nodeIds );

    //  Populate a set with disk nodes which are not DN
    NODEENTRYSET nodes;
    for ( DeviceManager::CITNODE_IDS itni = nodeIds.begin(); itni != nodeIds.end(); ++itni )
    {
        const DeviceManager::NodeEntry* pne = m_pDeviceManager->getNodeEntry( *itni );
        if ( pne )
        {
            if ( ( reinterpret_cast<const Device*>(pne->m_pNode)->getDeviceType() == Device::DeviceType::DISK )
                  && ( pne->m_Status != DeviceManager::NDST_DN ) )
                nodes.insert( pne );
        }
    }

    if ( !nodes.empty() )
        generateOutput( nodes );
}


//  handleAVAIL()
//
//  Handles FS,AVAIL
void
FSKeyin::handleAVAIL() const
{
    if ( m_Parameters.size() == 0 )
    {
        displayGeneralError();
        return;
    }

    std::set<DeviceManager::NODE_ID> nodeIds;
    for ( INDEX vx = 0; vx < m_Parameters.size(); ++vx )
    {
        const DeviceManager::NodeEntry* pNodeEntry = m_pDeviceManager->getNodeEntry( m_Parameters[vx] );
        if ( pNodeEntry == 0 )
        {
            notifyDoesNotExist( m_Parameters[vx] );
            return;
        }

        bool trouble = true;
        if ( pNodeEntry->isDevice() )
        {
            const Device* pDevice = reinterpret_cast<const Device*>(pNodeEntry->m_pNode);
            if ( pDevice->getDeviceType() == Device::DeviceType::DISK )
            {
                nodeIds.insert( pNodeEntry->m_NodeId );
                trouble = false;
            }
        }
        else if ( pNodeEntry->isController() )
        {
            const Controller* pController = reinterpret_cast<const Controller*>(pNodeEntry->m_pNode);
            if ( pController->getControllerType() == Controller::ControllerType::DISK )
            {
                DeviceManager::CITNODEIDMAP itChild = pNodeEntry->m_ChildNodeIds.begin();
                for ( ; itChild != pNodeEntry->m_ChildNodeIds.end(); ++itChild )
                    nodeIds.insert( itChild->second );
                trouble = false;
            }
        }

        if ( trouble )
        {
            std::string errmsg = "FS KEYIN - " + m_Parameters[vx] + " IS NOT A DISK CU OR DEVICE";
            m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
            return;
        }
    }

    std::set<DeviceManager::NODE_ID>::const_iterator itNodeId = nodeIds.begin();
    for ( ; itNodeId != nodeIds.end(); ++itNodeId )
    {
        const DeviceManager::NodeEntry* pNodeEntry = m_pDeviceManager->getNodeEntry( *itNodeId );
        if ( pNodeEntry )
            generateAvailabilityOutput( pNodeEntry );
    }
}


//  handleCM()
//
//  Handles FS,CM
void
FSKeyin::handleCM() const
{
    if ( m_Parameters.size() > 0 )
    {
        displayGeneralError();
        return;
    }

    DeviceManager::NODE_IDS nodeIds;
    m_pDeviceManager->getChannelModuleIdentifiers( &nodeIds );
    NODEENTRYSET nodes;
    for ( DeviceManager::CITNODE_IDS itni = nodeIds.begin(); itni != nodeIds.end(); ++itni )
    {
        const DeviceManager::NodeEntry* pne = m_pDeviceManager->getNodeEntry( *itni );
        if ( pne )
            nodes.insert( pne );
    }

    if ( !nodes.empty() )
        generateOutput( nodes );
}


//  handleComponent()
//
//  Simple query for the status of a single component
//      FS {componentName}
//  Because we are being all helpful here, we'll actually accept a list of component names.
void
FSKeyin::handleComponent() const
{
    NODEENTRYSET nodes;
    for ( INDEX vx = 0; vx < m_Parameters.size(); ++vx )
    {
        const DeviceManager::NodeEntry* pne = m_pDeviceManager->getNodeEntry( m_Parameters[vx] );
        if ( pne == 0 )
        {
            notifyDoesNotExist( m_Parameters[vx] );
            return;
        }

        nodes.insert( pne );
    }

    if ( nodes.size() > 0 )
        generateOutput( nodes );
    else
        displayGeneralError();
}


//  handleInterface()
//
//  Later versions of Exec have split the path component into upper and lower level interfaces.
//  We still use the older path nomenclature because... well... because shut up.
//      FS [ {iopName}/{cmName}/{cuName}/{devName} ]
//  Just for the heck of it, we'll accept empty strings as wildcards in any combination of the
//  four subfields and display multiple results.
void
FSKeyin::handleInterface() const
{
    const DeviceManager::NodeEntry* pProcEntry = 0;
    const DeviceManager::NodeEntry* pCMEntry = 0;
    const DeviceManager::NodeEntry* pCUEntry = 0;
    const DeviceManager::NodeEntry* pDevEntry = 0;

    DeviceManager::PROCESSOR_ID iopId = 0;
    DeviceManager::CHANNEL_MODULE_ID cmId = 0;
    DeviceManager::CONTROLLER_ID cuId = 0;
    DeviceManager::DEVICE_ID devId = 0;

    if ( m_Parameters.size() > 1 )
    {
        displayGeneralError();
        return;
    }
    else if ( m_Parameters.size() == 1 )
    {
        SuperString temp = m_Parameters[0];
        std::string iopName = temp.strip( '/' );
        std::string cmName = temp.strip( '/' );
        std::string cuName = temp.strip( '/' );
        if ( temp.find( '/' ) != temp.npos )
        {
            displayGeneralError();
            return;
        }
        std::string devName = temp;

        //???? generalize IS NOT A(n) messages...
        if ( iopName.size() > 0 )
        {
            pProcEntry = m_pDeviceManager->getNodeEntry( iopName );
            if ( pProcEntry == 0 )
            {
                notifyDoesNotExist( iopName );
                return;
            }

            if ( !pProcEntry->isProcessor() )
            {
                std::string errmsg = "FS KEYIN - " + iopName + " IS NOT AN IOP, INPUT IGNORED";
                m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
                return;
            }

            iopId = pProcEntry->m_NodeId;
        }

        if ( cmName.size() > 0 )
        {
            pCMEntry = m_pDeviceManager->getNodeEntry( cmName );
            if ( pCMEntry == 0 )
            {
                notifyDoesNotExist( cmName );
                return;
            }

            if ( !pCMEntry->isChannelModule() )
            {
                std::string errmsg = "FS KEYIN - " + cmName + " IS NOT A CHANNEL MODULE, INPUT IGNORED";
                m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
                return;
            }

            cmId = pCMEntry->m_NodeId;
        }

        if ( cuName.size() > 0 )
        {
            pCUEntry = m_pDeviceManager->getNodeEntry( cuName );
            if ( pCUEntry == 0 )
            {
                notifyDoesNotExist( cuName );
                return;
            }

            if ( !pCUEntry->isController() )
            {
                std::string errmsg = "FS KEYIN - " + cuName + " IS NOT A CONTROLLER, INPUT IGNORED";
                m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
                return;
            }

            cuId = pCUEntry->m_NodeId;
        }

        if ( devName.size() > 0 )
        {
            pDevEntry = m_pDeviceManager->getNodeEntry( devName );
            if ( pDevEntry == 0 )
            {
                notifyDoesNotExist( devName );
                return;
            }

            if ( !pDevEntry->isDevice() )
            {
                std::string errmsg = "FS KEYIN - " + devName + " IS NOT A DEVICE, INPUT IGNORED";
                m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
                return;
            }

            devId = pDevEntry->m_NodeId;
        }
    }

    //  Generate a list of device nodes in which we are interested.
    //  It will either be one, or many.
    DeviceManager::NODE_IDS nodeIds;
    if ( pDevEntry )
        nodeIds.push_back( devId );
    else
        m_pDeviceManager->getDeviceIdentifiers( &nodeIds );

    //  Now iterate over the list of interesting node IDs.
    //  Filter out any that do not match explicitly-given IOP, CM, or CU.
    //  Don't worry about matching DEV - we already did that when we created the container.
    for ( DeviceManager::CITNODE_IDS itni = nodeIds.begin(); itni != nodeIds.end(); ++itni )
    {
        const DeviceManager::DeviceEntry* pDevEntry =
            dynamic_cast<const DeviceManager::DeviceEntry*>(m_pDeviceManager->getNodeEntry( *itni ));
        if ( pDevEntry )
        {
            const DeviceManager::PathSet& pathSet = pDevEntry->m_PathSet;
            for ( DeviceManager::CITPATHS itPath = pathSet.m_Paths.begin(); itPath != pathSet.m_Paths.end(); ++itPath )
            {
                DeviceManager::PROCESSOR_ID chkIopId = (*itPath)->m_IOPIdentifier;
                DeviceManager::CHANNEL_MODULE_ID chkCmId = (*itPath)->m_ChannelModuleIdentifier;
                DeviceManager::CONTROLLER_ID chkCuId = (*itPath)->m_ControllerIdentifier;

                if ( (iopId != 0) && (iopId != chkIopId) )
                    continue;
                if ( (cmId != 0) && (cmId != chkCmId) )
                    continue;
                if ( (cuId != 0) && (cuId != chkCuId) )
                    continue;

                const DeviceManager::ProcessorEntry* pProcEntry = m_pDeviceManager->getProcessorEntry( chkIopId );
                const DeviceManager::ChannelModuleEntry* pCMEntry = m_pDeviceManager->getChannelModuleEntry( chkCmId );
                const DeviceManager::ControllerEntry* pCUEntry = m_pDeviceManager->getControllerEntry( chkCuId );

                if ( (pProcEntry == 0) || (pCMEntry == 0) || (pCUEntry == 0) )
                {
                    SystemLog::write( "FSKeyin trap 1" );
                    m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
                    return;
                }

                std::string msg = pProcEntry->m_pNode->getName() + "/"
                    + pCMEntry->m_pNode->getName() + "/"
                    + pCUEntry->m_pNode->getName() + "/"
                    + pDevEntry->m_pNode->getName() + " "
                    + DeviceManager::getNodeStatusString( (*itPath)->m_PathStatus );
                if ( !(*itPath)->m_Accessible )
                    msg += " NA";
                m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
            }
        }
    }
}


//  handleIOP()
//
//  Handles FS,IOP
void
FSKeyin::handleIOP() const
{
    if ( m_Parameters.size() > 0 )
    {
        displayGeneralError();
        return;
    }

    DeviceManager::NODE_IDS nodeIds;
    m_pDeviceManager->getProcessorIdentifiers( &nodeIds );
    NODEENTRYSET nodes;
    for ( DeviceManager::CITNODE_IDS itni = nodeIds.begin(); itni != nodeIds.end(); ++itni )
    {
        const DeviceManager::NodeEntry* pne = m_pDeviceManager->getNodeEntry( *itni );
        if ( pne )
        {
            const Processor* pProcessor = dynamic_cast<const Processor *>( pne->m_pNode );
            if ( pProcessor->getProcessorType() == Processor::ProcessorType::IOP )
                nodes.insert( pne );
        }
    }

    if ( !nodes.empty() )
        generateOutput( nodes );
}


//  handleMS()
//
//  Handles FS,MS
void
FSKeyin::handleMS() const
{
    if ( m_Parameters.size() > 0 )
    {
        displayGeneralError();
        return;
    }

    //  Get all device node ides
    DeviceManager::NODE_IDS nodeIds;
    m_pDeviceManager->getDeviceIdentifiers( &nodeIds );

    //  Develop set of nodes of the proper criteria
    NODEENTRYSET nodes;
    for ( DeviceManager::CITNODE_IDS itni = nodeIds.begin(); itni != nodeIds.end(); ++itni )
    {
        const DeviceManager::NodeEntry* pne = m_pDeviceManager->getNodeEntry( *itni );
        if ( pne && ( reinterpret_cast<const Device*>(pne->m_pNode)->getDeviceType() == Device::DeviceType::DISK ) )
            nodes.insert( pne );
    }

    if ( !nodes.empty() )
        generateOutput( nodes );
}


//  handlePACK()
//
//  Handles FS,PACK
void
FSKeyin::handlePACK() const
{
    //TODO:REM
    std::string errmsg = "REM DISK not yet implemented";
    m_pConsoleManager->postReadOnlyMessage( errmsg, m_Routing, m_pExec->getRunInfo() );
    /*
    PACK pack-id [/[dir-id],[pack-id] /[dir-id]...] or PACK [/ dir-id]
        The status, inhibit status, and assign count of removable disk packs in the list of
        packs in the UP, DN, or PRESENT status. The first format lets you specify a
        particular pack-id. The second format displays all UP and assigned, DN, or
        PRESENT removable packs.

For removable packs, the display format is
pack-id state - INHIBITS = inhibits ASSIGN COUNT = count dir-id HOSTS =
hosts
where:
pack-id
is the name of the pack from the FS,PACK devnam keyin or the pack-id for each
device on the FS,PACK keyin.
state
is UP, DN, or PRESENT.
inhibits
can be R (read), W (write), A (assign) or NONE (no inhibits assigned).
count
is the number of files assigned on the pack.
dir-id
is the directory-id, either STD or SHARED.
hosts
is a list of hosts that have files assigned on a shared pack in the DN state.

FS,PACK
Displays the status, inhibit status, and assign count of all removable disk packs in the
list of packs in the UP, DN, or PRESENT status, as follows:
91Y5 UP - INHIBITS = NONE ASSIGN COUNT = 8
91K5 UP - INHIBITS = NONE ASSIGN COUNT = 168

FS,PACK 91Y5
Displays the status, inhibit status, and assign count of removable disk pack 91Y5 in
the list of packs in the UP, DN, or PRESENT status, as follows:
91Y5 UP - INHIBITS = NONE ASSIGN COUNT = 8
If removable disk pack 91Y5 is not found in the system list of packs that are UP, DN,
or PRESENT, its status is reported as NOT PRESENT:
91Y5 NOT PRESENT - INHIBITS = NONE ASSIGN COUNT = 0

FS,PACK 91Y6/SHARED
Displays the status, inhibits, assign count, and host-id of hosts that have files
assigned on shared pack 91Y6, as follows:
91Y6 DN - INHIBITS = NONE ASSIGN COUNT = 8 SHARED HOSTS = AC


MESSAGES---------------------

FS NOT ALLOWED UNTIL MASS STORAGE INITIALIZED OR RECOVERED
(Exec) An FS,PACK keyin is not allowed until the recovery files have been created
or restored.

FS,PACK KEYIN ERROR - MHFS IS NOT AVAILABLE
(Exec) The FS,PACK/SHARED keyin is not allowed because Multi-Host File Sharing
(MHFS) is down or not available.

FS,PACK NOT ALLOWED - dir-id IS ILLEGAL DIRECTORY ID
(Exec) The directory supplied on the keyin was not STD, SHARED, or blank.
    */
}


//  isAllowed()
//
//  FS keyin is always allowed (except FS,PACK - see that code)
bool
FSKeyin::isAllowed() const
{
    return true;
}



// constructors / destructors

FSKeyin::FSKeyin
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

