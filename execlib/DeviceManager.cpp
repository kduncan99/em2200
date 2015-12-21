//	DeviceManager.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Implementation of DeviceManager class



#include	"execlib.h"



//	statics



//  private methods

//  setNodeDown()
//
//  Sets node down, and takes care of any consequences thereof.
void
DeviceManager::setNodeDown
(
    Activity* const     pActivity,
    NodeEntry* const    pNodeEntry
)
{
    pNodeEntry->m_Status = NDST_DN;

    //  Now go re-evaluate accessibility of all nodes
    NODEENTRYSET updatedNodes;
    evaluateAccessibility( &updatedNodes );

    //  Any assigned disk drives gone inaccessible or DN due to this?
    if ( !pNodeEntry->isDevice() )
    {
        //TODO:REM need to ask fac manager for assigned RV disks as well...
        NODE_IDS asgNodes;
        MFDManager* pMfdMgr = dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) );
        pMfdMgr->getAssignedNodeIds( &asgNodes );
        for ( NODE_IDS::const_iterator itNode = asgNodes.begin(); itNode != asgNodes.end(); ++itNode )
        {
            NodeEntry* pEntry = m_NodeEntries[*itNode];
            if ( (pEntry->m_Status == NDST_DN) || !pEntry->m_Accessible )
            {
                SystemLog::write("DeviceManager::setNodeDown():Assigned disk becoming inaccessible");
                m_pExec->stopExec( Exec::SC_RESPONSE_REQUIRES_REBOOT );
                break;
            }
        }
    }
}


//  setNodeReserved()
//
//  Sets node reserved, and takes care of any consequences.
void
DeviceManager::setNodeReserved
(
    Activity* const     pActivity,
    NodeEntry* const    pNodeEntry
)
{
    //????
}


//  setNodeSuspended()
//
//  Sets node suspended, and takes care of any consequences.
//  Since only disk devices can be SU, we fully expect the node to be a disk device.
void
DeviceManager::setNodeSuspended
(
    Activity* const     pActivity,
    NodeEntry* const    pNodeEntry
    )
{
    //  If currently UP, nothing much to do but set new status.
    //  If currently DN or RV, tell MFD to bring the pack online.
    if ( pNodeEntry->m_Status == NDST_UP )
        pNodeEntry->m_Status = NDST_SU;
    else
    {
        MFDManager* const pMFDManager = dynamic_cast<MFDManager*>(m_pExec->getManager( Exec::MID_MFD_MANAGER ));
        if ( pMFDManager == 0 )
        {
            SystemLog::write( "DeviceManager::setNodeSuspended() Cannot find MFDManager" );
            m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
            return;
        }

        MFDManager::Result mfdResult = pMFDManager->bringPackOnline( pActivity, pNodeEntry->m_NodeId );
        if ( mfdResult.m_Status == MFDManager::MFDST_SUCCESSFUL )
            pNodeEntry->m_Status = NDST_SU;
    }
}


//  setNodeUp()
//
//  Sets node up, and takes care of any consequences.
void
DeviceManager::setNodeUp
(
    Activity* const     pActivity,
    NodeEntry* const    pNodeEntry
)
{
    //????
}



//  Constructors, destructors

DeviceManager::DeviceManager
(
    Exec* const         pExec
)
:ExecManager( pExec )
{
    m_NextNodeId = 1;
}


DeviceManager::~DeviceManager()
{
}



//  Public methods

//  cleanup()
//
//  Called when Exec is being unloaded (just before delete of Manager objects)
void
DeviceManager::cleanup()
{
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
        delete itne->second;
    m_NodeEntries.clear();
}


//  dump()
//
//  For debugging
void
DeviceManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "DeviceManager ----------" << std::endl;
    stream << "  Nodes:" << std::endl;
    for (CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        const NodeEntry* pEntry = itne->second;
        const Node* pNode = pEntry->m_pNode;
        stream << "    [" << pEntry->m_NodeId << "] "
            << pNode->getName()
            << " " << getNodeStatusAndAvailabilityString(pEntry->m_Status)
            << "  " << Node::getCategoryString( pNode->getCategory() );
        switch ( pNode->getCategory() )
        {
            case Node::Category::CHANNEL_MODULE:
                break;
            case Node::Category::CONTROLLER:
            {
                const Controller* pController = reinterpret_cast<const Controller*>( pNode );
                stream << "  " << Controller::getControllerTypeString( pController->getControllerType() );
                break;
            }
            case Node::Category::DEVICE:
            {
                const Device* pDevice = reinterpret_cast<const Device*>( pNode );
                stream << "  " << Device::getDeviceTypeString( pDevice->getDeviceType() )
                    << "  " << Device::getDeviceModelString( pDevice->getDeviceModel() );
                break;
            }
            case Node::Category::PROCESSOR:
            {
                const Processor* pProcessor = reinterpret_cast<const Processor*>( pNode );
                stream << "  " << Processor::getProcessorTypeString( pProcessor->getProcessorType() );
                break;
            }
        }
        stream << std::endl;

        stream << "      Parent Node Ids:";
        if ( pEntry->m_ParentNodeIds.size() == 0 )
            stream << "  None";
        else
        {
            for ( INDEX px = 0; px < pEntry->m_ParentNodeIds.size(); ++px )
                stream << "  " << std::dec << " " << pEntry->m_ParentNodeIds[px];
        }
        stream << std::endl;

        stream << "      Child Node [Addr]=Ids:";
        if ( pEntry->m_ChildNodeIds.size() == 0 )
            stream << "  None";
        else
        {
            for ( CITNODEIDMAP itChild = pEntry->m_ChildNodeIds.begin(); itChild != pEntry->m_ChildNodeIds.end(); ++itChild )
                stream << "[" << std::dec << itChild->first << "]=" << itChild->second << "  ";
        }
        stream << std::endl;

        if ( pEntry->isDevice() )
        {
            const DeviceEntry* pDevEntry = static_cast<const DeviceEntry*>(pEntry);
            const PATHS& paths = pDevEntry->m_PathSet.m_Paths;
            stream << "      [{addr}][{nodeId}]{name}/..." << std::endl;
            for ( CITPATHS itp = paths.begin(); itp != paths.end(); ++itp )
            {
                const Path* pPath = *itp;
                stream << "      " << std::dec
                    << "[" << pPath->m_IOPUPINumber
                    << "][" << pPath->m_IOPIdentifier
                    << "]" << getNodeName( pPath->m_IOPIdentifier )
                    << "/[" << pPath->m_ChannelModuleAddress
                    << "][" << pPath->m_ChannelModuleIdentifier
                    << "]"  <<  getNodeName( pPath->m_ChannelModuleIdentifier )
                    << "/[" << pPath->m_ControllerAddress
                    << "][" << pPath->m_ControllerIdentifier
                    << "]" << getNodeName( pPath->m_ControllerIdentifier )
                    << "/[" << pPath->m_DeviceAddress
                    << "][" << pPath->m_DeviceIdentifier
                    << "]" << getNodeName( pPath->m_DeviceIdentifier )
                    << "  " << getNodeStatusString( pPath->m_PathStatus )
                    << (pPath->m_Accessible ? "" : "(NA)")
                    << std::endl;
            }
        }
    }
}


//  getChannelModuleCount()
COUNT
DeviceManager::getChannelModuleCount() const
{
    lock();
    COUNT count = getCount( Node::Category::CHANNEL_MODULE );
    unlock();
    return count;
}


//  getChannelModuleEntry()
const DeviceManager::ChannelModuleEntry*
DeviceManager::getChannelModuleEntry
(
    const CHANNEL_MODULE_ID     channelModuleIdentifier
) const
{
    lock();
    const ChannelModuleEntry* pEntry = dynamic_cast<const ChannelModuleEntry*>( getEntry( channelModuleIdentifier ) );
    unlock();
    return pEntry;
}


//  getChannelModuleIdentifiers()
void
DeviceManager::getChannelModuleIdentifiers
(
    NODE_IDS* const     pContainer
) const
{
    pContainer->clear();
    lock();

    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isChannelModule() )
            pContainer->push_back( itne->first );
    }

    unlock();
}


//  getControllerCount()
COUNT
DeviceManager::getControllerCount() const
{
    lock();
    COUNT count = getCount( Node::Category::CONTROLLER );
    unlock();
    return count;
}


//  getControllerEntry()
const DeviceManager::ControllerEntry*
DeviceManager::getControllerEntry
(
    const CONTROLLER_ID         controllerIdentifier
) const
{
    lock();
    const ControllerEntry* pEntry = dynamic_cast<const ControllerEntry*>( getEntry( controllerIdentifier ) );
    unlock();
    return pEntry;
}


//  getControllerIdentifiers()
void
DeviceManager::getControllerIdentifiers
(
    NODE_IDS* const     pContainer
) const
{
    pContainer->clear();
    lock();

    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isController() )
            pContainer->push_back( itne->first );
    }

    unlock();
}


//  getDeviceCount()
COUNT
DeviceManager::getDeviceCount() const
{
    lock();
    COUNT count = getCount( Node::Category::DEVICE );
    unlock();
    return count;
}


//  getDeviceEntry()
const DeviceManager::DeviceEntry*
DeviceManager::getDeviceEntry
(
    const DEVICE_ID         deviceIdentifier
) const
{
    lock();
    const DeviceEntry* pEntry = dynamic_cast<const DeviceEntry*>( getEntry( deviceIdentifier ) );
    unlock();
    return pEntry;
}


//  getDeviceEntry()
const DeviceManager::DeviceEntry*
DeviceManager::getDeviceEntry
(
    const std::string&      nodeName
) const
{
    lock();
    const DeviceEntry* pEntry = dynamic_cast<const DeviceEntry*>( getEntry( nodeName ) );
    unlock();
    return pEntry;
}


//  getDeviceIdentifiers()
void
DeviceManager::getDeviceIdentifiers
(
    NODE_IDS* const     pContainer
) const
{
    pContainer->clear();
    lock();

    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isDevice() )
            pContainer->push_back( itne->first );
    }

    unlock();
}


void
DeviceManager::getDeviceIdentifiers
(
    NODE_IDS* const             pContainer,
    const Device::DeviceType    deviceType
) const
{
    pContainer->clear();
    lock();

    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isDevice() )
        {
            const Device* pDevice = reinterpret_cast<const Device*>( itne->second );
            if (pDevice->getDeviceType() == deviceType)
            {
                pContainer->push_back( itne->first );
            }
        }
    }

    unlock();
}


//  getProcessorCount()
COUNT
DeviceManager::getProcessorCount() const
{
    lock();
    COUNT count = getCount( Node::Category::PROCESSOR );
    unlock();
    return count;
}


//  getProcessorEntry()
const DeviceManager::ProcessorEntry*
DeviceManager::getProcessorEntry
(
    const PROCESSOR_ID          ioProcessorIdentifier
) const
{
    lock();
    const ProcessorEntry* pEntry = dynamic_cast<const ProcessorEntry*>( getEntry( ioProcessorIdentifier ) );
    unlock();
    return pEntry;
}


//  getIOProcessorIdentifiers()
void
DeviceManager::getProcessorIdentifiers
(
    NODE_IDS* const     pContainer
) const
{
    pContainer->clear();
    lock();

    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isProcessor() )
            pContainer->push_back( itne->first );
    }

    unlock();
}


//  getNextPath()
//
//  Retrieves a pointer to the next usable path to the indicated device.
//  Path must be UP and accessible.
const DeviceManager::Path*
DeviceManager::getNextPath
(
    const DEVICE_ID     deviceIdentifier
) const
{
    CITNODEENTRIES itne = m_NodeEntries.find( deviceIdentifier );
    assert( itne != m_NodeEntries.end() );//???? exec stop instead
    DeviceEntry* pEntry = dynamic_cast<DeviceEntry*>(itne->second);
    assert( pEntry );//???? exec stop instead
    return pEntry->m_PathSet.getNextPath();
}


//  getNodeEntry()
//
//  External wrapper around the internal function which converts a NODE_ID to a base const NodeEntry object.
const DeviceManager::NodeEntry*
DeviceManager::getNodeEntry
(
    const NODE_ID       nodeId
) const
{
    lock();
    const NodeEntry* pne = getEntry( nodeId );
    unlock();
    return pne;
}


//  getNodeEntry()
//
//  External wrapper around the internal function which locates whichever NodeEntry corresponds to the given node name.
const DeviceManager::NodeEntry*
DeviceManager::getNodeEntry
(
    const std::string&      nodeName
) const
{
    lock();
    const NodeEntry* pne = getEntry( nodeName );
    unlock();
    return pne;
}


//  getNodeIdentifier()
//
//  Returns the NODE_ID associated with the given Node object
DeviceManager::NODE_ID
DeviceManager::getNodeIdentifier
(
    const Node* const   pNode
) const
{
    NODE_ID nodeId = 0;

    lock();
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode == pNode )
        {
            nodeId = itne->first;
            break;
        }
    }

    unlock();
    return nodeId;
}


//  getNodeName()
//
//  Retrieves a reference to a std::string containing the indicated node's name.
//  Does NOT validate nodeId - will die badly if you pass a bad one.
const std::string&
DeviceManager::getNodeName
(
    const NODE_ID nodeId
) const
{
    lock();
    const std::string& ref = getEntry( nodeId )->m_pNode->getName();
    unlock();

    return ref;
}


//  getNodeStatusAndAvailabilityString()
//
//  pretty much what it says
std::string
DeviceManager::getNodeStatusAndAvailabilityString
(
    const NODE_ID   nodeId
) const
{
    lock();
    const NodeEntry* pne = getEntry( nodeId );
    std::string result = getNodeStatusString( pne->m_Status );
    if ( !pne->m_Accessible )
        result += " NA";
    unlock();
    return result;
}


//  setNodeDownInternally()
//
//  Some bit of Exec or other has decided to automatically DN a component.
//  Report it, do it, then figure out any cascading effects, stopping the EXEC if necessary.
void
DeviceManager::setNodeDownInternally
(
    Activity* const     pActivity,
    const NODE_ID       nodeId
)
{
    std::stringstream logStrm;
    logStrm << "DeviceManager setNodeDownInternally node=" << nodeId;
    SystemLog::write( logStrm.str() );

    NodeEntry* pEntry = getEntry( nodeId );
    if ( pEntry == 0 )
    {
        //  This is bad - stop the EXEC
        std::stringstream logStrm;
        logStrm << "Fatal:setNodeDownInternally() called with bad nodeId = " << nodeId;
        SystemLog::getInstance()->write( logStrm.str() );
        m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
        return;
    }

    std::string msg = "DN OF " + pEntry->m_pNode->getName() + " Internal request was received";
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( msg, m_pExec->getRunInfo() );

    setNodeDown( pActivity, pEntry );

    msg = pEntry->m_pNode->getName() + " DN";
    pConsMgr->postReadOnlyMessage( msg, m_pExec->getRunInfo() );
}


//  setNodeStatus()
//
//  Prevents callers from using the non-activity version which is exposed by the base class
void
DeviceManager::setNodeStatus
(
    const NODE_ID       nodeId,
    const NodeStatus    nodeStatus
)
{
    assert(false);
}


//  setNodeStatus()
//
//  Sets a node UP, DN, RV, or SU as requested by the corresponding facility keyin.
//  As much console interaction as possible is pushed out to the keyin classes.
//  If we get called, we presume all verification is done and the operator really wants to do this.
void
DeviceManager::setNodeStatus
(
    Activity* const     pActivity,
    const NODE_ID       nodeId,
    const NodeStatus    nodeStatus
)
{
    lock();

    //  Find NodeEntry object
    NodeEntry* pNodeEntry = getEntry( nodeId );
    if ( pNodeEntry == 0 )
    {
        SystemLog::write( "DeviceManager::setNodeStatus() called with bad nodeId" );
        m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
        unlock();
        return;
    }

    //  If the node is not already in the requested state, delegate to an appropriate sub-function.
    if ( nodeStatus != pNodeEntry->m_Status )
    {
        switch ( nodeStatus )
        {
        case NDST_DN:
            setNodeDown( pActivity, pNodeEntry );
            break;

        case NDST_RV:
            setNodeReserved( pActivity, pNodeEntry );
            break;

        case NDST_SU:
            setNodeSuspended( pActivity, pNodeEntry );
            break;

        case NDST_UP:
            setNodeUp( pActivity, pNodeEntry );
            break;
        }
    }

    unlock();
}


//  shutdown()
//
//  Exec would like to shut down
void
DeviceManager::shutdown()
{
    SystemLog::write( "DeviceManager::shutdown()" );
}


//  startup()
//
//  Initializes the manager - Exec is booting
bool
DeviceManager::startup()
{
    SystemLog::write( "DeviceManager::startup()" );

    //  Load nodes (only on the first boot)
    const std::set<Node*>& nodeSet = m_pExec->getNodeTable().getNodeSet();
    if ( m_NodeEntries.size() == 0 )
    {
        //  If node entry table is empty, this is probably the first boot for this Exec instance.
        //  Populate the node entry table based on the node tree from the configuration object.
        //  Create path information for the device entry objects.
        for ( std::set<Node*>::const_iterator itn = nodeSet.begin(); itn != nodeSet.end(); ++itn )
        {
            Node* pNode = *itn;
            createNodeEntry( pNode );
            /* obsolete????
            m_pExec->postNodeChangeEvent( pNode );
            */
        }

        //  Build parent and child node-id linkages based on detected hardware linkages.
        for ( ITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
        {
            NodeEntry* pParentEntry = itne->second;
            Node* pParentNode = pParentEntry->m_pNode;
            const Node::DESCENDANTS& childNodes = pParentNode->getDescendants();
            for ( auto itcn = childNodes.begin(); itcn != childNodes.end(); ++itcn )
            {
                Node::NODE_ADDRESS childAddress = itcn->first;
                NodeEntry* pChildEntry = getEntry( itcn->second );
                pParentEntry->m_ChildNodeIds[childAddress] = pChildEntry->m_NodeId;
                pChildEntry->m_ParentNodeIds.push_back( pParentEntry->m_NodeId );
            }
        }

        //  Now create a PathSet for each device node entry by walking the ancestry tree down from the IOPs.
        for ( ITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
        {
            ProcessorEntry* pProcessorEntry = dynamic_cast<ProcessorEntry*>( itne->second );
            if ( pProcessorEntry )
            {
                PROCESSOR_UPI iopUPI = pProcessorEntry->m_UPI;
                NODE_ID iopNodeId = pProcessorEntry->m_NodeId;
                for ( ITNODEIDMAP itchmod = pProcessorEntry->m_ChildNodeIds.begin(); itchmod != pProcessorEntry->m_ChildNodeIds.end(); ++itchmod )
                {
                    Node::NODE_ADDRESS cmAddress = itchmod->first;
                    NODE_ID cmNodeId = itchmod->second;
                    ChannelModuleEntry* pCMEntry = dynamic_cast<ChannelModuleEntry*>(getEntry( cmNodeId ));
                    for ( ITNODEIDMAP itctl = pCMEntry->m_ChildNodeIds.begin(); itctl != pCMEntry->m_ChildNodeIds.end(); ++itctl )
                    {
                        Node::NODE_ADDRESS ctlAddress = itctl->first;
                        NODE_ID ctlNodeId = itctl->second;
                        ControllerEntry* pCtlEntry = dynamic_cast<ControllerEntry*>(getEntry( ctlNodeId ));
                        for ( ITNODEIDMAP itdev = pCtlEntry->m_ChildNodeIds.begin(); itdev != pCtlEntry->m_ChildNodeIds.end(); ++itdev )
                        {
                            Node::NODE_ADDRESS devAddress = itdev->first;
                            NODE_ID devNodeId = itdev->second;
                            DeviceEntry* pDevEntry = dynamic_cast<DeviceEntry*>(getEntry( devNodeId ) );

                            Path* pPath = new Path( iopNodeId, iopUPI, cmNodeId, cmAddress, ctlNodeId, ctlAddress, devNodeId, devAddress );
                            pDevEntry->m_PathSet.addPath( pPath );
                        }
                    }
                }
            }
        }

        //  Set up accessibility...
        NODEENTRYSET nodes;
        evaluateAccessibility( &nodes );
    }

    //  Set SKDATA for all channel modules
    bool skipDataFlag = m_pExec->getConfiguration().getBoolValue( "SKDATA" );
    for (ITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->isChannelModule() )
        {
            ChannelModule* pchmod = dynamic_cast<ChannelModule*>(itne->second->m_pNode);
            pchmod->setSkipDataFlag( skipDataFlag );
        }
    }

    return true;
}


//  terminate()
//
//  Final termination of exec
void
DeviceManager::terminate()
{
    SystemLog::write( "DeviceManager::terminate()" );
}



//  Subordinate class methods

//  createFromBuffer()
//
//  Given the 36-bit buffer resulting from an INQUIRY command to a disk device,
//  we extract the disk pack info and populate a dynamically-allocated DiskDeviceInfo36
//  object and return it to the caller.
//  Expects inquiry data to have been read via IOXLAT_C mode.
//
//  The given buffer MUST be at least 28 words in length.
DeviceManager::DiskDeviceInfo36*
DeviceManager::DiskDeviceInfo36::createFromBuffer
(
    const Word36* const     pBuffer
)
{
    BYTE byteBuffer[128];
    miscWord36Pack( byteBuffer, pBuffer, 28 );
    const DiskDevice::DiskDeviceInfo* pddInfo = reinterpret_cast<const DiskDevice::DiskDeviceInfo*>(byteBuffer);
    return new DiskDeviceInfo36( pddInfo->m_DeviceInfo.m_DeviceModel,
                                 pddInfo->m_DeviceInfo.m_DeviceType,
                                 pddInfo->m_BlockCount,
                                 pddInfo->m_BlockSize,
                                 pddInfo->m_IsMounted,
                                 pddInfo->m_IsReady,
                                 pddInfo->m_IsWriteProtected );
}


//  addPath()
//
//  Adds a path to a PathSet
void
DeviceManager::PathSet::addPath
(
    Path* const         pPath
)
{
    m_Paths.push_back( pPath );
    m_LastPathUsed = m_Paths.size() - 1;
}


//  getNextPath()
//
//  Retrieves the next usable path from the PathSet.
//  Returns 0 if there is no usable path.
const DeviceManager::Path*
DeviceManager::PathSet::getNextPath()
{
    const Path* pResult = 0;

    if ( m_Paths.size() > 0 )
    {
        INDEX firstCheckPath = m_LastPathUsed;
        ++firstCheckPath;
        if ( firstCheckPath == m_Paths.size() )
            firstCheckPath = 0;
        INDEX checkPath = firstCheckPath;

        while ( pResult == 0 )
        {
            if ( (m_Paths[checkPath]->m_PathStatus == NDST_UP) && m_Paths[checkPath]->m_Accessible )
            {
                pResult = m_Paths[checkPath];
                m_LastPathUsed = checkPath;
            }
            else
            {
                ++checkPath;
                if ( checkPath == m_Paths.size() )
                    checkPath = 0;
                if ( checkPath == firstCheckPath )
                    break;
            }
        }
    }

    return pResult;
}



//  public statics

const std::string
    DeviceManager::getNodeStatusString
    (
    const NodeStatus    status
    )
{
    switch ( status )
    {
    case NDST_DN:   return "DN";
    case NDST_RV:   return "RV";
    case NDST_SU:   return "SU";
    case NDST_UP:   return "UP";
    }

    return "???";
}

