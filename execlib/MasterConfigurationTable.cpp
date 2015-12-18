//  MasterConfigurationTable shenanigans



#include    "execlib.h"



//  private, protected methods

//  createChannelModuleEntry()
//
//  Creates a ChannelModuleEntry object for the given node
MasterConfigurationTable::ChannelModuleEntry*
MasterConfigurationTable::createChannelModuleEntry
(
    ChannelModule* const    pChannelModule
)
{
    return createChannelModuleEntry( m_NextNodeId++, pChannelModule );
}


//  createChannelModuleEntry()
//
//  Creates a ChannelModuleEntry object for the given node
MasterConfigurationTable::ChannelModuleEntry*
MasterConfigurationTable::createChannelModuleEntry
(
    const CHANNEL_MODULE_ID channelModuleId,
    ChannelModule* const    pChannelModule
)
{
    ChannelModuleEntry* pEntry = new ChannelModuleEntry( channelModuleId, pChannelModule );
    m_NodeEntries[channelModuleId] = pEntry;
    return pEntry;
}


//  createControllerEntry()
//
//  Creates a ControllerEntry object for the given node
MasterConfigurationTable::ControllerEntry*
MasterConfigurationTable::createControllerEntry
(
    Controller* const       pController
)
{
    return createControllerEntry( m_NextNodeId++, pController );
}


//  createControllerEntry()
//
//  Creates a ControllerEntry object for the given node
MasterConfigurationTable::ControllerEntry*
MasterConfigurationTable::createControllerEntry
(
    const CONTROLLER_ID     controllerId,
    Controller* const       pController
)
{
    ControllerEntry* pEntry = new ControllerEntry( controllerId, pController );
    m_NodeEntries[controllerId] = pEntry;
    return pEntry;
}


//  createDeviceEntry()
//
//  Creates a DeviceEntry object for the given node
MasterConfigurationTable::DeviceEntry*
MasterConfigurationTable::createDeviceEntry
(
    Device* const           pDevice
)
{
    return createDeviceEntry( m_NextNodeId++, pDevice );
}


//  createDeviceEntry()
//
//  Creates a DeviceEntry object for the given node
MasterConfigurationTable::DeviceEntry*
MasterConfigurationTable::createDeviceEntry
(
    const DEVICE_ID         deviceId,
    Device* const           pDevice
)
{
    SuperString modelName( "EMFSD" );    //  TODO:HWCONFIG later, we base this on model type
    DeviceEntry* pEntry = new DeviceEntry( deviceId, pDevice, modelName );
    m_NodeEntries[deviceId] = pEntry;
    return pEntry;
}


//  createIOProcessorEntry()
//
//  Creates an IOProcessorEntry object for the given node
MasterConfigurationTable::ProcessorEntry*
MasterConfigurationTable::createProcessorEntry
(
    Processor* const        pProcessor
)
{
    return createProcessorEntry( m_NextNodeId++, pProcessor );
}


//  createIOProcessorEntry()
//
//  Creates an IOProcessorEntry object for the given node
MasterConfigurationTable::ProcessorEntry*
MasterConfigurationTable::createProcessorEntry
(
    const PROCESSOR_ID      processorId,
    Processor* const        pProcessor
)
{
    //  Note: IOP UPI is the same as processorId.  This might change some day, but it works for now.
    ProcessorEntry* pEntry = new ProcessorEntry( processorId, pProcessor, static_cast<PROCESSOR_UPI>(processorId) );
    m_NodeEntries[processorId] = pEntry;
    return pEntry;
}


//  createNodeEntry()
//
//  Creates a new NodeEntry object for the given node
MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::createNodeEntry
(
    Node* const             pNode
)
{
    switch ( pNode->getCategory() )
    {
    case Node::Category::CHANNEL_MODULE:    return createChannelModuleEntry( dynamic_cast<ChannelModule*>(pNode) );
    case Node::Category::CONTROLLER:        return createControllerEntry( dynamic_cast<Controller*>(pNode) );
    case Node::Category::DEVICE:            return createDeviceEntry( dynamic_cast<Device*>(pNode) );
    case Node::Category::PROCESSOR:         return createProcessorEntry( dynamic_cast<Processor*>(pNode) );
    }

    assert(false);
    return 0;
}


//  evaluateAccessibility()
//
//  Evaluate the entire node tree to see which (if any) nodes of the indicated category
//  have changed accessibility.  The updated container will be the set of all changed nodes.
void
MasterConfigurationTable::evaluateAccessibility
(
    const Node::Category    category,
    NODEENTRYSET* const     pUpdatedNodes
) const
{
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        NodeEntry* pne = itne->second;
        if ( pne->m_pNode->getCategory() == category )
        {
            bool originalValue = pne->m_Accessible;
            bool newValue = isNodeAccessible( pne );
            if ( originalValue != newValue )
            {
                pne->m_Accessible = newValue;
                pUpdatedNodes->insert( pne );
            }
        }
    }
}


//  evaluatePaths()
//
//  (Re)-determine the accessibility of all the paths for all the devices.
//  To be invoked after one or more node statuses have changed.
void
MasterConfigurationTable::evaluatePaths() const
{
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        NodeEntry* pne = itne->second;
        if ( pne->isDevice() )
        {
            DeviceEntry* pde = reinterpret_cast<DeviceEntry*>( pne );
            for ( INDEX px = 0; px < pde->m_PathSet.m_Paths.size(); ++px )
            {
                Path* path = pde->m_PathSet.m_Paths[px];
                const ProcessorEntry* ppe =
                    reinterpret_cast<const ProcessorEntry*>( getEntry( path->m_IOPIdentifier ) );
                const ChannelModuleEntry* pcme =
                    reinterpret_cast<const ChannelModuleEntry*>( getEntry( path->m_ChannelModuleIdentifier ) );
                const ControllerEntry* pce =
                    reinterpret_cast<const ControllerEntry*>( getEntry( path->m_ControllerIdentifier ) );
                path->m_Accessible = ppe->m_Accessible
                                        && (ppe->m_Status == NodeStatus::NDST_UP)
                                        && pcme->m_Accessible
                                        && (pcme->m_Status == NodeStatus::NDST_UP)
                                        && pce->m_Accessible
                                        && (pce->m_Status == NodeStatus::NDST_UP);
            }
        }
    }
}


//  getCount()
//
//  Counts the number of nodes for a particular category - prefer calling under lock().
COUNT
MasterConfigurationTable::getCount
(
    const Node::Category        category
) const
{
    COUNT count = 0;
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode->getCategory() == category )
            ++count;
    }

    return count;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node id.
//  Currently the container of node entry objects is a vector, which makes this fairly primitive.
//  However, we might change the container to a map at some point, in which case this makes more sense.
const MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    const NODE_ID           nodeId
) const
{
    const NodeEntry* pne = 0;
    CITNODEENTRIES itne = m_NodeEntries.find( nodeId );
    if ( itne != m_NodeEntries.end() )
        pne = itne->second;
    return pne;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node id.
//  Currently the container of node entry objects is a vector, which makes this fairly primitive.
//  However, we might change the container to a map at some point, in which case this makes more sense.
MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    const NODE_ID           nodeId
)
{
    NodeEntry* pne = 0;
    ITNODEENTRIES itne = m_NodeEntries.find( nodeId );
    if ( itne != m_NodeEntries.end() )
        pne = itne->second;
    return pne;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node pointer.
const MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    Node* const             pNode
) const
{
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode == pNode )
            return itne->second;
    }

    return 0;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node pointer.
MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    Node* const             pNode
)
{
    for ( ITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode == pNode )
            return itne->second;
    }

    return 0;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node pointer.
const MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    const std::string&      nodeName
) const
{
    for ( CITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode->getName().compareNoCase( nodeName ) == 0 )
            return itne->second;
    }

    return 0;
}


//  getEntry()
//
//  Retrieves the entry pointer associated with the given node pointer.
MasterConfigurationTable::NodeEntry*
MasterConfigurationTable::getEntry
(
    const std::string&      nodeName
)
{
    for ( ITNODEENTRIES itne = m_NodeEntries.begin(); itne != m_NodeEntries.end(); ++itne )
    {
        if ( itne->second->m_pNode->getName().compareNoCase( nodeName ) == 0 )
            return itne->second;
    }

    return 0;
}


//  isNodeAccessible()
//
//  Real-time evaluation of the accessibility of a node
bool
MasterConfigurationTable::isNodeAccessible
(
    const NodeEntry* const      pEntry
) const
{
    //  If this is an IOP, it is always accessible
    if ( pEntry->isProcessor() )
        return true;

    //  If at least one parent is up and accessible, we're accessible.
    //  We don't have to check SU and RV; only devices are SU/RV, and they are never parents.
    CITNODE_IDS itParentNodeId = pEntry->m_ParentNodeIds.begin();
    bool found = false;
    while ( !found && (itParentNodeId != pEntry->m_ParentNodeIds.end()) )
    {
        const NodeEntry* pParentEntry = getEntry( *itParentNodeId );
        if ( (pParentEntry->m_Status == NDST_UP) && (isNodeAccessible( pParentEntry )) )
            found = true;
        ++itParentNodeId;
    }

    return found;
}



//  constructors, destructors

//  special constructor for doing a medium-deep copy of an MCT to produce a new one for what-ifs.
MasterConfigurationTable::MasterConfigurationTable
(
    const MasterConfigurationTable& source
)
{
    for ( CITNODEENTRIES itSource = source.m_NodeEntries.begin(); itSource != source.m_NodeEntries.end(); ++itSource )
    {
        NODE_ID sourceNodeId = itSource->first;
        const NodeEntry* const pSourceNodeEntry = itSource->second;
        if ( pSourceNodeEntry->isChannelModule() )
        {
            ChannelModule* pChannelModule = dynamic_cast<ChannelModule*>( pSourceNodeEntry->m_pNode );
            createChannelModuleEntry( sourceNodeId, pChannelModule );
        }
        else if ( pSourceNodeEntry->isController() )
        {
            Controller* pController = dynamic_cast<Controller*>( pSourceNodeEntry->m_pNode );
            createControllerEntry( sourceNodeId, pController );
        }
        else if ( pSourceNodeEntry->isDevice() )
        {
            const DeviceEntry* pSourceDeviceEntry = dynamic_cast<const DeviceEntry*>( pSourceNodeEntry );
            Device* pDevice = dynamic_cast<Device*>( pSourceNodeEntry->m_pNode );
            DeviceEntry* pDestDeviceEntry = createDeviceEntry( sourceNodeId, pDevice );
            const PATHS sourcePaths = pSourceDeviceEntry->m_PathSet.m_Paths;
            PATHS destPaths = pDestDeviceEntry->m_PathSet.m_Paths;
            for ( CITPATHS itSourcePath = sourcePaths.begin(); itSourcePath != sourcePaths.end(); ++itSourcePath )
                destPaths.push_back( new Path( **itSourcePath ) );
        }
        else if ( pSourceNodeEntry->isProcessor() )
        {
            Processor* pProcessor = dynamic_cast<Processor*>( pSourceNodeEntry->m_pNode );
            createProcessorEntry( sourceNodeId, pProcessor );
        }
    }
}


MasterConfigurationTable::~MasterConfigurationTable()
{
    while ( !m_NodeEntries.empty() )
    {
        delete m_NodeEntries.begin()->second;
        m_NodeEntries.erase( m_NodeEntries.begin()->first );
    }
}


//  public methods

//  evaluateAccessibility()
//
//  Evaluate the entire node tree to see which (if any) nodes have changed in accessibility.
//  For this to work properly, we need to work from the top downward.
//  The updated container will be the set of all changed nodes.
void
MasterConfigurationTable::evaluateAccessibility
(
    NODEENTRYSET* const     pUpdatedNodes
) const
{
    evaluateAccessibility( Node::Category::PROCESSOR, pUpdatedNodes );
    evaluateAccessibility( Node::Category::CHANNEL_MODULE, pUpdatedNodes );
    evaluateAccessibility( Node::Category::CONTROLLER, pUpdatedNodes );
    evaluateAccessibility( Node::Category::DEVICE, pUpdatedNodes );
    evaluatePaths();
}


//  setNodeStatus()
//
//  Updates the status for the indicated node.  Be SURE not to call unless you are SURE the node exists.
void
MasterConfigurationTable::setNodeStatus
(
    const NODE_ID       nodeIdentifier,
    const NodeStatus    nodeStatus
)
{
    NodeEntry* pne = getEntry( nodeIdentifier );
    pne->m_Status = nodeStatus;
}
