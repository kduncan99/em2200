//  MasterConfigurationTable.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  An Exec view of the hardware.
//  Wraps hardware Node entities in NodeEntry objects, which have exec status attributes, and which maintains
//  path information, along with path attributes.



#ifndef     EMEXEC_MASTER_CONFIGURATION_TABLE_H
#define     EMEXEC_MASTER_CONFIGURATION_TABLE_H



class   MasterConfigurationTable
{
public:
    typedef INDEX                   NODE_ID;                //  Index into our master node table for the following entities
    typedef NODE_ID                 CHANNEL_MODULE_ID;
    typedef NODE_ID                 CONTROLLER_ID;
    typedef NODE_ID                 DEVICE_ID;
    typedef NODE_ID                 PROCESSOR_ID;

    typedef INDEX                   PATH_ID;

    //  A container of NODE_IDs
    typedef std::vector<NODE_ID>                    NODE_IDS;
    typedef NODE_IDS::iterator                      ITNODE_IDS;
    typedef NODE_IDS::const_iterator                CITNODE_IDS;

    //  Maps NODE_ADDRESS to NODE_ID
    typedef std::map<Node::NODE_ADDRESS, NODE_ID>   NODEIDMAP;
    typedef NODEIDMAP::iterator                     ITNODEIDMAP;
    typedef NODEIDMAP::const_iterator               CITNODEIDMAP;

    enum NodeStatus
    {
        NDST_DN,
        NDST_RV,
        NDST_SU,
        NDST_UP,
    };

    //  Path information -------------------------------------------------------
    class   Path
    {
    public:
        const PROCESSOR_ID              m_IOPIdentifier;
        const PROCESSOR_UPI             m_IOPUPINumber;
        const CHANNEL_MODULE_ID         m_ChannelModuleIdentifier;
        const Node::NODE_ADDRESS        m_ChannelModuleAddress;
        const CONTROLLER_ID             m_ControllerIdentifier;
        const Node::NODE_ADDRESS        m_ControllerAddress;
        const DEVICE_ID                 m_DeviceIdentifier;
        const Node::NODE_ADDRESS        m_DeviceAddress;
        NodeStatus						m_PathStatus;
        bool                            m_Accessible;       //  false if any component in the path is DN

        Path( const PROCESSOR_ID            iopIdentifier,
              const PROCESSOR_UPI           iopUpiNumber,
              const CHANNEL_MODULE_ID       channelModuleIdentifier,
              const Node::NODE_ADDRESS      channelModuleAddress,
              const CONTROLLER_ID           controllerIdentifier,
              const Node::NODE_ADDRESS      controllerAddress,
              const DEVICE_ID               deviceIdentifier,
              const Node::NODE_ADDRESS      deviceAddress)
             :m_IOPIdentifier(iopIdentifier),
             m_IOPUPINumber(iopUpiNumber),
             m_ChannelModuleIdentifier(channelModuleIdentifier),
             m_ChannelModuleAddress(channelModuleAddress),
             m_ControllerIdentifier(controllerIdentifier),
             m_ControllerAddress(controllerAddress),
             m_DeviceIdentifier(deviceIdentifier),
             m_DeviceAddress(deviceAddress),
             m_PathStatus(NDST_UP)
        {}
    };

    typedef     std::vector<Path*>          PATHS;
    typedef     PATHS::iterator             ITPATHS;
    typedef     PATHS::const_iterator       CITPATHS;

    class   PathSet
    {
    public:
        PATHS                           m_Paths;
        INDEX                           m_LastPathUsed;

        PathSet()
            :m_LastPathUsed(0)
        {}

        ~PathSet()
        {
            for ( PATHS::iterator itp = m_Paths.begin(); itp != m_Paths.end(); ++itp )
                delete *itp;
        }

        void                addPath( Path* const pPath );
        const Path*         getNextPath();
    };

    //  Node information -------------------------------------------------------
    class   NodeEntry
    {
    public:
        bool                    m_Accessible;       //  False if there is no path to any IOP
        Node* const             m_pNode;            //  We do NOT own this object, so we do not delete it.
        const NODE_ID           m_NodeId;
        NodeStatus              m_Status;
        NODEIDMAP               m_ChildNodeIds;     //  keyed by Node::NODE_ADDRESS
        NODE_IDS                m_ParentNodeIds;    //  not keyed

    protected:
        NodeEntry( const NODE_ID        nodeId,
                   Node* const          pNode )
                  :m_pNode( pNode ),
                  m_NodeId( nodeId ),
                  m_Status( NDST_UP )
        {}

    public:
        virtual ~NodeEntry(){}

        virtual bool    isChannelModule() const { return false; }
        virtual bool    isController() const { return false; }
        virtual bool    isDevice() const { return false; }
        virtual bool    isProcessor() const { return false; }
    };

    typedef     std::map<NODE_ID, NodeEntry*>               NODEENTRIES;
    typedef     NODEENTRIES::iterator                       ITNODEENTRIES;
    typedef     NODEENTRIES::const_iterator                 CITNODEENTRIES;

    typedef     std::list<NodeEntry*>                       NODEENTRYLIST;
    typedef     NODEENTRYLIST::iterator                     ITNODEENTRYLIST;
    typedef     NODEENTRYLIST::const_iterator               CITNODEENTRYLIST;

    typedef     std::set<NodeEntry*>                        NODEENTRYSET;
    typedef     NODEENTRYSET::iterator                      ITNODEENTRYSET;
    typedef     NODEENTRYSET::const_iterator                CITNODEENTRYSET;

    class   ChannelModuleEntry : public NodeEntry
    {
    public:
        const std::string   m_Name;

        ChannelModuleEntry( const NODE_ID           nodeId,
                            ChannelModule* const    pChannelModule)
            :NodeEntry( nodeId, pChannelModule )
        {}

        bool        isChannelModule() const { return true; }
    };

    class   ControllerEntry : public NodeEntry
    {
    public:
        ControllerEntry( const NODE_ID      nodeId,
                         Controller* const  pController)
            :NodeEntry( nodeId, pController )
        {}

        bool        isController() const { return true; }
    };

    class   DeviceEntry : public NodeEntry
    {
    public:
        const SuperString       m_EquipmentModelName;
        PathSet                 m_PathSet;

        DeviceEntry( const NODE_ID          nodeId,
                     Device* const          pDevice,
                     const SuperString&     equipmentModelName )
                    :NodeEntry( nodeId, pDevice ),
                    m_EquipmentModelName( equipmentModelName )
        {
        }

        bool        isDevice() const { return true; }
    };

    class   ProcessorEntry : public NodeEntry
    {
    public:
        const PROCESSOR_UPI     m_UPI;

        ProcessorEntry( const NODE_ID       nodeId,
                        Processor* const    pProcessor,
                        const PROCESSOR_UPI upi )
            :NodeEntry( nodeId, pProcessor ),
            m_UPI( upi )
        {}

        ~ProcessorEntry() {}

        bool    isProcessor() const { return true; }
    };


protected:
    NODE_ID                         m_NextNodeId;
    NODEENTRIES                     m_NodeEntries;

    ChannelModuleEntry*             createChannelModuleEntry( ChannelModule* const pNode );
    ChannelModuleEntry*             createChannelModuleEntry( const CHANNEL_MODULE_ID   channelModuleId,
                                                              ChannelModule* const      pNode );
    ControllerEntry*                createControllerEntry( Controller* const pNode );
    ControllerEntry*                createControllerEntry( const CONTROLLER_ID  controllerId,
                                                           Controller* const    pNode );
    DeviceEntry*                    createDeviceEntry( Device* const pNode );
    DeviceEntry*                    createDeviceEntry( const DEVICE_ID  deviceId,
                                                       Device* const    pNode );
    ProcessorEntry*                 createProcessorEntry( Processor* const pNode );
    ProcessorEntry*                 createProcessorEntry( const PROCESSOR_ID    processorId,
                                                          Processor* const      pNode );
    NodeEntry*                      createNodeEntry( Node* const pNode );
    void                            evaluateAccessibility( Node::Category       category,
                                                           NODEENTRYSET* const  pUpdatedNodes) const;
    void                            evaluatePaths() const;
    COUNT                           getCount( const Node::Category category ) const;
    NodeEntry*                      getEntry( const NODE_ID nodeId );
    const NodeEntry*                getEntry( const NODE_ID nodeId ) const;
    NodeEntry*                      getEntry( Node* const pNode );
    const NodeEntry*                getEntry( Node* const pNode ) const;
    NodeEntry*                      getEntry( const std::string& nodeName );
    const NodeEntry*                getEntry( const std::string& nodeName ) const;
    bool                            isNodeAccessible( const NodeEntry* const pNodeEntry ) const;

    MasterConfigurationTable(){};

public:
    MasterConfigurationTable( const MasterConfigurationTable& source );
    virtual ~MasterConfigurationTable();

    void                            evaluateAccessibility( NODEENTRYSET* const pUpdatedNodes ) const;
    virtual void                    setNodeStatus( const NODE_ID    nodeId,
                                                   const NodeStatus nodeStatus );
};



#endif

