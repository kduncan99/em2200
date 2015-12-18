//  DeviceManager.h header file
//
//  The DeviceManager acts as a sort of gate-keeper for everything traditionally related
//  to the Master Configuration Table.  This would seem to be in the domain of
//  FacilitiesManager, but that class is busy handling the next level higher, so we
//  have percolated all stuff actually pertaining to bits of (virtual) hardware to here.



#ifndef     EMEXEC_DEVICE_MANAGER_H
#define     EMEXEC_DEVICE_MANAGER_H



#include    "MasterConfigurationTable.h"



class   DeviceManager : public ExecManager,
                        public MasterConfigurationTable
{
public:
    //  Device Info developed from CHCMD_INQUIRY commands ----------------------
    class   DeviceInfo36
    {
    public:
        const Device::DeviceModel       m_Model;
        const Device::DeviceType        m_Type;

        DeviceInfo36(const Device::DeviceModel  model,
                     const Device::DeviceType   type)
                     :m_Model(model),
                     m_Type(type)
        {}
    };

    class   DiskDeviceInfo36 : public DeviceInfo36
    {
    public:
        const BLOCK_COUNT       m_BlockCount;
        const BLOCK_SIZE        m_BlockSize;
        const bool              m_IsMounted;
        const bool              m_IsReady;
        const bool              m_IsWriteProtected;

        DiskDeviceInfo36(const Device::DeviceModel  model,
                         const Device::DeviceType   type,
                         const BLOCK_COUNT          blockCount,
                         const BLOCK_SIZE           blockSize,
                         const bool                 isMounted,
                         const bool                 isReady,
                         const bool                 isWriteProtected)
                         :DeviceInfo36(model, type),
                         m_BlockCount(blockCount),
                         m_BlockSize(blockSize),
                         m_IsMounted(isMounted),
                         m_IsReady(isReady),
                         m_IsWriteProtected(isWriteProtected)
        {}

        static DiskDeviceInfo36*    createFromBuffer( const Word36* const pBuffer );
    };


private:
    void                            setNodeDown( Activity* const    pActivity,
                                                 NodeEntry* const   pNode );
    void                            setNodeReserved( Activity* const    pActivity,
                                                     NodeEntry* const   pNode );
    void                            setNodeSuspended( Activity* const   pActivity,
                                                      NodeEntry* const  pNode );
    void                            setNodeUp( Activity* const  pActivity,
                                               NodeEntry* const pNode );

public:
    DeviceManager( Exec* const pExec );
    ~DeviceManager();

    COUNT                           getChannelModuleCount() const;
    const ChannelModuleEntry*       getChannelModuleEntry( const CHANNEL_MODULE_ID channelModuleIdentifier ) const;
    void                            getChannelModuleIdentifiers( NODE_IDS* const pContainer ) const;
    COUNT                           getControllerCount() const;
    const ControllerEntry*          getControllerEntry( const CONTROLLER_ID controllerIdentifier ) const;
    void                            getControllerIdentifiers( NODE_IDS* const pContainer ) const;
    COUNT                           getDeviceCount() const;
    const DeviceEntry*              getDeviceEntry( const DEVICE_ID deviceIdentifier ) const;
    const DeviceEntry*              getDeviceEntry( const std::string& nodeName ) const;
    void                            getDeviceIdentifiers( NODE_IDS* const pContainer ) const;
    void                            getDeviceIdentifiers( NODE_IDS* const           pContainer,
                                                          const Device::DeviceType  nodeType ) const;
    COUNT                           getProcessorCount() const;
    const ProcessorEntry*           getProcessorEntry( const PROCESSOR_ID processorIdentifier ) const;
    void                            getProcessorIdentifiers( NODE_IDS* const pContainer ) const;
    const Path*                     getNextPath( const DEVICE_ID deviceIdentifier ) const;
    const NodeEntry*                getNodeEntry( const NODE_ID nodeIdentifier ) const;
    const NodeEntry*                getNodeEntry( const std::string& nodeName ) const;
    NODE_ID                         getNodeIdentifier( const Node* const pNode ) const;
    const std::string&              getNodeName( const NODE_ID nodeId ) const;
    std::string                     getNodeStatusAndAvailabilityString( const NODE_ID nodeId ) const;
    void                            setNodeDownInternally( Activity* const  pActivity,
                                                           const NODE_ID    nodeId );
    void                            setNodeStatus( const NODE_ID    nodeId,
                                                   const NodeStatus nodeStatus );
    void                            setNodeStatus( Activity* const  pActivity,
                                                   const NODE_ID    nodeId,
                                                   const NodeStatus nodeStatus );

    //  ExecManager interface
    void                            cleanup();
    void                            dump( std::ostream&     stream,
                                          const DUMPBITS    dumpBits );
    void                            shutdown();
    bool                            startup();
    void                            terminate();

    //  statics
    static const std::string        getNodeStatusString( const NodeStatus status);
};



#endif
