//	DNKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the DN keyin


/*  FORMATS
To bring down a component, use the following keyin format:
DN component[,com2,com3,...]

To bring down an interface, use the following keyin format:
DN cmod/[intf#]/cu[,cmod//cu2,...]
This keyin makes the interface between the specified channel module (cmod) and
the control unit (cu) unavailable. If you do not specify an interface number (intf#), the
Exec supplies a value. You can also use this keyin for bringing down an interface to
the record lock processor (RLP).

To bring down all devices attached to a control unit, use the following keyin format:
DN,ALL cu[,cu2,cu3,...]

To disable XPC caching, use one of the following keyin formats:
DN,XCACHE xpcname
DN,XCACHE xpcname/LOCAL
DN,XCACHE xpcname/SHARED
where xpcname is the logical name of the XPC.
Use the DN,XCACHE xpcname keyin to disable local and shared caching for each host
using the XPC.

Use the DN,XCACHE xpcname/LOCAL keyin to disable a host's local caching using the
XPC. If local caching is disabled using this keyin, local caching can only be enabled using
an UP,XCACHE xpcname/LOCAL keyin.

Use the DN,XCACHE xpcname/SHARED keyin to disable shared caching using the XPC.

To bring down one or more removable disk packs, use the following keyin format:
DN,PACK pack-id[/[dir-id]//[eqp-mnemonic],[pack-id]...]
where:
dir-id
is the directory identifier (SHARED or STD).
eqp-mnemonic
is the mnemonic name of the unit the pack is on.
This keyin places the specified packs in the DN state. Refer to Section 5. This keyin
should only be used by those with a very specific environment.
Note: For use of the DN,PACK keyin in a local environment, the NEWCON
configuration parameter REQNBRPST0 must not be set to 0. In a shared
environment, the parameter REQNBRPST1 must not be set to 0. See the Exec
Installation and Configuration Guide for more information on these configuration
parameters.
*/

/*  RESTRICTIONS
Restrictions
When you bring down a fixed disk with a status of UP or SU, or when you bring down a
disk that is both reserved and assigned, you lose access to all files on that disk. A
message is displayed that requires you to respond Y or N. Reply Y to continue
processing the keyin; this results in an XER 055 system stop. Reply N to tell the system
to ignore the keyin.

When you bring down a removable disk that is assigned, a XER 055 system stop occurs
and the status change occurs after the next disk boot. When you bring down a
removable disk that is not assigned, the status changes immediately and no XER 055
system stop occurs.

When you bring down an active printer, the device stops immediately. If you want to
allow the current activity to finish, lock out the device (see the description of the SM
device L keyin) before bringing it down.

You cannot bring down the last processor.

Bringing down a tape drive that is assigned to a run causes the run to abort.

You cannot use the DN,PACK keyin on unit-duplexed devices, but you can use the DN
keyin on one of the devices to break the Unit Duplexing association.

When you bring down the last XIIP on an XPC, the XPC also becomes unavailable (NA).

If a DN IOP results in no path available to the XPC FO, a new type and reply message is
displayed as follows:
IOP name is connected to <VIchannelname>.
DN OF <IOPname> will disable this host's last path to XPC name.
Continue to DN, enter DISABLE, else CANCEL.
If you answer DISABLE to the message, EXEC continues the DN of IOP. Otherwise, the
EXEC aborts processing of the DN IOP.
*/

/*  MESSAGES
    DN device
    (Exec) This message appears when the device is brought down internally, which
    happens after a Fibre Channel SCSI Console informational message has informed
    the user that a device conflict status has occurred.

    DN device FATAL, CONTINUE PROCESSING KEYIN? Y OR N
    (Exec) An attempt has been made to bring down a fixed disk or the last console
    CRT.

    DN console FATAL, WILL MAKE crt NA, CONTINUE? Y/N
    (Exec) Your console will become unavailable if you use the DN keyin.
    Y makes the console unavailable.
    N cancels the DN keyin.

    DN component FATAL - WILL MAKE component NA, CONTINUE KEYIN? Y/N
    (Exec) Bringing the component down will make it unavailable.
    Y brings down the component.
    N cancels the DN keyin.

    DN KEYIN - (ALL) OPTION INVALID FOR component, INPUT IGNORED
    (Exec) You cannot use the DN,ALL keyin on the specified component.

    DN Keyin already performed for component
    (Exec) The specified component is already down (not available).

    DN KEYIN - component DOES NOT EXIST, INPUT IGNORED
    (Exec) The specified component is not configured.

    DN KEYIN FAILED - I/O ERROR PROCESSING KEYIN FOR pack-id
    (Exec) An I/O error occurred while processing the keyin; the keyin is ignored.

    DN KEYIN - WARNING-device ASSIGNED, EXECUTE? Y N
    (Exec) You see this message after a DN keyin to a device that was assigned. If
    you answer Y, some system degradation and possibly the loss of a run may occur.
    If you answer N, the DN keyin is ignored.

    DN OF component FAILED DUE TO INTERNAL DATA CORRUPTION
    (Exec) Data in the Exec internal memory table has been damaged or destroyed.

    DN OF ctrl-unit FAILED: UNABLE TO RELEASE UNIT
    (Exec) The tape unit on the DN keyin cannot be released. Before a tape unit can be
    brought down, it must be released so it can be accessed by other systems. If the
    unit is successfully brought down, control of the LED is returned to the hardware.

    DN OF component name Internal request was received
    (Exec) This message is displayed when the system has requested bringing
    down component name due to an internal failure.
    This message will also be displayed during IOP initialization if some of the
    configured channels are inoperable.

    DN OF device IS FATAL, CONTINUE PROCESSING KEYIN? Y OR N
    (Exec) A system reboot is required if you bring down the last system console,
    or if you initialize or bring down a fixed mass storage device.
    Y executes the keyin.
    N terminates the keyin.

    DN OF component IS NOT ALLOWED
    (Exec) The DN keyin you entered is not allowed for one of the following reasons:
        Incompatible equipment
        Keyin would make system disk unavailable

    DN OF device NOT ALLOWED DURING TAPE BOOT
    (Exec) You tried to bring a device down during an initial boot or a tape recovery boot.
    You can bring down the unit after SYS FIN. device is the logical name of the boot
    tape unit.

    DN OF name NOT ALLOWED: WILL MAKE device NA DURING TAPE BOOT
    (Exec) This message is received during an initial boot or a tape recovery boot if you
    try to bring down a component that would render the boot tape unit not available.
    It is displayed if the keyin is fatal to all hosts and the device is the shared system
    device. This restriction does not apply after SYS FIN.

    DN OF component NOT PERFORMED - KEYIN ABORTED
    (Exec) You responded N to a previous message to perform a DN keyin.

    DN OF device NOT PERFORMED - PACK VERIFICATION ACTIVE
    (Exec) You tried to bring down a disk device while the pack is being verified.
    Wait until pack verification is complete and try the keyin again.

    DN component WILL MAKE ctrl-unit NA, CONTINUE KEYIN? Y/N
    (Exec) You tried to bring down a system component that makes a communications
    control unit inaccessible, or possibly cause the loss of a communications network.
    component is the system component in the DN keyin.
    ctrl-unit is the communications control unit that will be NA (not available).
*/



#include	"execlib.h"



//	private / protected methods

//  handleComponentList()
//
//  We are given a list of components as candidates to be brought DN.
//  Analyze the consequences, verify with the operator as necessary, and optionally take the action.
void
    DNKeyin::handleComponentList()
{
    //  Create copy of MCT so we can play what-if games.
    MasterConfigurationTable workingMCT( *m_pDeviceManager );
    for ( INDEX px = 0; px < m_Parameters.size(); ++px )
    {
        const DeviceManager::NodeEntry* pNodeEntry = m_pDeviceManager->getNodeEntry( m_Parameters[px] );
        if ( pNodeEntry == 0 )
        {
            notifyDoesNotExist( m_Parameters[px] );
            return;
        }

        workingMCT.setNodeStatus( pNodeEntry->m_NodeId, MasterConfigurationTable::NDST_DN );
        MasterConfigurationTable::NODEENTRYSET updatedNodes;
        workingMCT.evaluateAccessibility( &updatedNodes );
        for ( MasterConfigurationTable::CITNODEENTRYSET itne = updatedNodes.begin(); itne != updatedNodes.end(); ++itne )
        {
            //  Post prompts for all devices which are going DN or NA...
            /*
    DN device FATAL, CONTINUE PROCESSING KEYIN? Y OR N
    (Exec) An attempt has been made to bring down a fixed disk or the last console
    CRT.

    DN console FATAL, WILL MAKE crt NA, CONTINUE? Y/N
    (Exec) Your console will become unavailable if you use the DN keyin.
    Y makes the console unavailable.
    N cancels the DN keyin.

    DN component FATAL - WILL MAKE component NA, CONTINUE KEYIN? Y/N
    (Exec) Bringing the component down will make it unavailable.
    Y brings down the component.
    N cancels the DN keyin.

    DN Keyin already performed for component
    (Exec) The specified component is already down (not available).

    DN KEYIN - component DOES NOT EXIST, INPUT IGNORED
    (Exec) The specified component is not configured.

    DN KEYIN - WARNING-device ASSIGNED, EXECUTE? Y N
    (Exec) You see this message after a DN keyin to a device that was assigned. If
    you answer Y, some system degradation and possibly the loss of a run may occur.
    If you answer N, the DN keyin is ignored.

    DN OF component FAILED DUE TO INTERNAL DATA CORRUPTION
    (Exec) Data in the Exec internal memory table has been damaged or destroyed.

    DN OF ctrl-unit FAILED: UNABLE TO RELEASE UNIT
    (Exec) The tape unit on the DN keyin cannot be released. Before a tape unit can be
    brought down, it must be released so it can be accessed by other systems. If the
    unit is successfully brought down, control of the LED is returned to the hardware.

    DN OF device IS FATAL, CONTINUE PROCESSING KEYIN? Y OR N
    (Exec) A system reboot is required if you bring down the last system console,
    or if you initialize or bring down a fixed mass storage device.
    Y executes the keyin.
    N terminates the keyin.

    DN OF component IS NOT ALLOWED
    (Exec) The DN keyin you entered is not allowed for one of the following reasons:
        Incompatible equipment
        Keyin would make system disk unavailable

    DN OF device NOT ALLOWED DURING TAPE BOOT
    (Exec) You tried to bring a device down during an initial boot or a tape recovery boot.
    You can bring down the unit after SYS FIN. device is the logical name of the boot
    tape unit.

    DN OF name NOT ALLOWED: WILL MAKE device NA DURING TAPE BOOT
    (Exec) This message is received during an initial boot or a tape recovery boot if you
    try to bring down a component that would render the boot tape unit not available.
    It is displayed if the keyin is fatal to all hosts and the device is the shared system
    device. This restriction does not apply after SYS FIN.

    DN OF component NOT PERFORMED - KEYIN ABORTED
    (Exec) You responded N to a previous message to perform a DN keyin.

    DN component WILL MAKE ctrl-unit NA, CONTINUE KEYIN? Y/N
    (Exec) You tried to bring down a system component that makes a communications
    control unit inaccessible, or possibly cause the loss of a communications network.
    component is the system component in the DN keyin.
    ctrl-unit is the communications control unit that will be NA (not available).
            */
        }
    }
}


//  handleControllerList()
//
//  We are given a list of controllers, the subordinate devices of which are all to be brought DN.
//  Analyze the consequences, verity with the operator as necessary, and optionally take the action.
void
    DNKeyin::handleControllerList()
{
    if ( m_Parameters.size() == 0 )
    {
        displayGeneralError();
        return;
    }

    //  Build set of devices which are affected
    NODEENTRYSET devices;
    for ( INDEX px = 0; px < m_Parameters.size(); ++px )
    {
        const DeviceManager::NodeEntry* pNodeEntry = m_pDeviceManager->getNodeEntry( m_Parameters[px] );
        if ( pNodeEntry == 0 )
        {
            notifyDoesNotExist( m_Parameters[px] );
            return;
        }

        if ( !pNodeEntry->isController() )
        {
            std::string msg = "DN KEYIN - (ALL) OPTION INVALID FOR " + m_Parameters[px] + ", INPUT IGNORED";
            m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
            return;
        }

        for ( DeviceManager::CITNODEIDMAP itChild = pNodeEntry->m_ChildNodeIds.begin();
            itChild != pNodeEntry->m_ChildNodeIds.end(); ++itChild )
        {
            const DeviceManager::NodeEntry* pChild = m_pDeviceManager->getNodeEntry( itChild->second );
            if ( !pChild )
            {
                std::stringstream strm;
                strm << "DNKeyin::handleControllerList cannot resolve child Node ID "
                    << itChild->second << " of parent " << pNodeEntry->m_pNode->getName();
                SystemLog::write(strm.str());
                std::string msg = "DN OF " + pNodeEntry->m_pNode->getName() + " FAILED DUE TO INTERNAL DATA CORRUPTION";
                m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
                return;
            }

            devices.insert( pChild );
        }
    }

    //  Check the individual devices to see if bringing any of them down would cause trouble.
    //  If so, verify that the operator wishes to proceed.
    for ( ITNODEENTRYSET itNode = devices.begin(); itNode != devices.end(); )
    {
        if ( (*itNode)->m_Status == DeviceManager::NDST_DN )
        {
            notifyKeyinAlreadyPerformed( *itNode );
            itNode = devices.erase( itNode );
            continue;
        }

        if ( !verifyDevice( *itNode ) )
        {
            notifyKeyinAborted( *itNode );
            return;
        }

        ++itNode;
    }

    //  Carry out the request
    for ( CITNODEENTRYSET itNode = devices.begin(); itNode != devices.end(); ++itNode )
        m_pDeviceManager->setNodeStatus( this, (*itNode)->m_NodeId, DeviceManager::NDST_DN );

    //  Finally, display the results...
    generateOutput( devices );
}


//  handleInterface()
//
//  There is one parameter, and it contains at least one slash (it should have three).
void
    DNKeyin::handleInterface() const
{
    SuperString tempString = m_Parameters[0];
    std::string iopName = tempString.strip( '/' );
    std::string cmName = tempString.strip( '/' );
    std::string cuName = tempString.strip( '/' );
    std::string devName = tempString;

    if ( iopName.empty() || cmName.empty() || cuName.empty() || devName.empty() )
    {
        displayGeneralError();
        return;
    }

    DeviceManager::PROCESSOR_ID procId = 0;
    DeviceManager::CHANNEL_MODULE_ID cmId = 0;
    DeviceManager::CONTROLLER_ID cuId = 0;
    DeviceManager::DEVICE_ID devId = 0;

    const DeviceManager::NodeEntry* pProcEntry = m_pDeviceManager->getNodeEntry( iopName );
    if ( pProcEntry && pProcEntry->isProcessor() )
        procId = pProcEntry->m_NodeId;

    const DeviceManager::NodeEntry* pCMEntry = m_pDeviceManager->getNodeEntry( cmName );
    if ( pCMEntry && pCMEntry->isChannelModule() )
        cmId = pCMEntry->m_NodeId;

    const DeviceManager::NodeEntry* pCUEntry = m_pDeviceManager->getNodeEntry( cuName );
    if ( pCUEntry && pCUEntry->isController() )
        cuId = pCUEntry->m_NodeId;

    const DeviceManager::DeviceEntry* pDevEntry =
        dynamic_cast<const DeviceManager::DeviceEntry*>( m_pDeviceManager->getNodeEntry( devName ) );
    if ( pDevEntry )
        devId = pDevEntry->m_NodeId;

    DeviceManager::Path* pPath = 0;
    if ( (procId != 0) && (cmId != 0) && (cuId != 0) && (devId != 0) )
    {
        //  Look through the device node to find the particular path, if it exists
        for ( DeviceManager::CITPATHS itp = pDevEntry->m_PathSet.m_Paths.begin();
            itp != pDevEntry->m_PathSet.m_Paths.end(); ++itp )
        {
            if ( ((*itp)->m_IOPIdentifier == pProcEntry->m_NodeId)
                && ((*itp)->m_ChannelModuleIdentifier == pCMEntry->m_NodeId)
                && ((*itp)->m_ControllerIdentifier == pCUEntry->m_NodeId) )
            {
                //  Found the path
                pPath = *itp;
                break;
            }
        }
    }

    //  No such path?
    if ( pPath == 0 )
    {
        notifyDoesNotExist( m_Parameters[0] );
        return;
    }

    //  If the path is already down, notify the operator and abort
    if ( pPath->m_PathStatus == DeviceManager::NDST_DN )
    {
        std::string msg = "PATH " + m_Parameters[0] + " IS ALREADY DN";
        m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
        return;
    }

    //  Will this render the device inaccessible?  If so, check with the operator before proceeding.
    bool hasOtherAvailablePath = false;
    for ( DeviceManager::CITPATHS itp = pDevEntry->m_PathSet.m_Paths.begin();
        itp != pDevEntry->m_PathSet.m_Paths.end(); ++itp )
    {
        if ( (pPath != *itp) && (*itp)->m_Accessible )
        {
            hasOtherAvailablePath = true;
            break;
        }
    }

    if ( !hasOtherAvailablePath )
    {
        //????
    }

    //  DN the path...
    //????
}


//  handlePackList()
//
//  For DN,PACK keyin
void
    DNKeyin::handlePackList() const
{
    /*
    To bring down one or more removable disk packs, use the following keyin format:
    DN,PACK pack-id[/[dir-id]//[eqp-mnemonic],[pack-id]...]
    where:
    dir-id
    is the directory identifier (SHARED or STD).
    eqp-mnemonic
    is the mnemonic name of the unit the pack is on.
    This keyin places the specified packs in the DN state. Refer to Section 5. This keyin
    should only be used by those with a very specific environment.
    Note: For use of the DN,PACK keyin in a local environment, the NEWCON
    configuration parameter REQNBRPST0 must not be set to 0. In a shared
    environment, the parameter REQNBRPST1 must not be set to 0. See the Exec
    Installation and Configuration Guide for more information on these configuration
    parameters.
    */
    m_pConsoleManager->postReadOnlyMessage( "REM PACK support not yet implemented",
                                            m_Routing,
                                            m_pExec->getRunInfo() );//TODO:REM
}


//	handler()
//
//	Called from Keyin base class worker() function
void
    DNKeyin::handler()
{
    if ( m_Option.size() == 0 )
    {
        if ( (m_Parameters.size() == 1) && (m_Parameters[0].find( '/' ) != m_Parameters[0].npos) )
            handleInterface();
        else if ( m_Parameters.size() > 1 )
            handleComponentList();
        else
            displayGeneralError();
    }
    else
    {
        if ( m_Option.compareNoCase( "ALL" ) == 0 )
            handleControllerList();
        else if ( m_Option.compareNoCase( "PACK" ) == 0 )
            handlePackList();
        else
            displayGeneralError();
    }
}


//  isAllowed()
bool
    DNKeyin::isAllowed() const
{
    Exec::Status execStatus = m_pExec->getStatus();
    return (execStatus == Exec::ST_BOOTING_1) || (execStatus == Exec::ST_RUNNING);
}


//  verifyDevice()
//
//  Checks the indicated device to see whether bringing it down will cause trouble.
//  In some cases it may reject the request out of hand; in others, it may defer to the console operator.
//
//  Returns true if the command should proceed, false to abort the command.
bool
    DNKeyin::verifyDevice
    (
    const DeviceManager::NodeEntry* const   pDeviceEntry
    ) const
{
    Device* pDevice = reinterpret_cast<Device*>( pDeviceEntry->m_pNode );
    if ( !pDevice )
    {
        SystemLog::write( "DNKeyin::verifyDevice() pDevice is null" );
        m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
        return false;
    }

    switch ( pDevice->getDeviceType() )
    {
        case Device::DeviceType::DISK:
        {
            const MFDManager::PackInfo* pPackInfo = m_pMFDManager->getPackInfo( pDeviceEntry->m_NodeId );
            if ( pPackInfo && (pPackInfo->m_AssignCount > 0) )
            {
                std::string prompt = "DN OF " + pDeviceEntry->m_pNode->getName() + " IS FATAL, CONTINUE PROCESSING KEYIN? Y OR N";
                PollResult pollResult = pollOperator( prompt );
                if ( (pollResult == PR_NO) || (pollResult == PR_CANCELED) )
                    return false;
            }
            return true;
        }

        case Device::DeviceType::SYMBIONT:
        {
            /*TODO:SYMB (local terminals)
                DN OF component IS NOT ALLOWED
                (Exec) The DN keyin you entered is not allowed for one of the following reasons:
                    Incompatible equipment
                    Keyin would make system disk unavailable
            */
            return true;
        }

        case Device::DeviceType::TAPE:
        {
            /*TODO:TAPE (and tape boot... if that even makes sense?)
            DN KEYIN - WARNING-device ASSIGNED, EXECUTE? Y N
            (Exec) You see this message after a DN keyin to a device that was assigned. If
            you answer Y, some system degradation and possibly the loss of a run may occur.
            If you answer N, the DN keyin is ignored.
            */

            /*TODO:TAPE and tape boot... if that even makes sense?
            DN OF device NOT ALLOWED DURING TAPE BOOT
            (Exec) You tried to bring a device down during an initial boot or a tape recovery boot.
            You can bring down the unit after SYS FIN. device is the logical name of the boot
            tape unit.
            */
            return true;
        }
    }

    SystemLog::write( "DNKeyin::verifyDevice() impossible fall-through" );
    m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
    return false;
}



// constructors / destructors

DNKeyin::DNKeyin
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

