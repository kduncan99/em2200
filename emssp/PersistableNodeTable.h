//  PersistableNodeTable.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Extends NodeTable to add persistence
//  as well as introducing the idea of configuration by subsystem.



#ifndef     EMSSP_PERSISTABLE_NODE_TABLE_H
#define     EMSSP_PERSISTABLE_NODE_TABLE_H



#include    "SubSystem.h"



class   PersistableNodeTable : public NodeTable, public Lockable
{
public:
    //  Result return by all atomic operations
    enum class Result
    {
        Success,
        AlreadyConnected,
        ChannelModuleStillConnected,
        FileCloseFailed,
        FileIoFailed,
        FileOpenFailed,
        GeneratedNameConflict,
        MountFailed,
        NameIsInvalid,
        NameIsNotUnique,
        NodeDoesNotExist,
        ParseError,
        TypeConflict,
    };

    typedef std::map<std::string, SubSystem*>   SUBSYSTEMS;
    typedef std::map<std::string, IOProcessor*> IOPROCESSORS;


private:
    bool                        m_IsUpdated;
    IOPROCESSORS                m_IoProcessors;
    SUBSYSTEMS                  m_SubSystems;

    bool                areNodeNamesUnique( const std::vector<SuperString>& names ) const;
    Result              createDeviceInt( SubSystem* const           pSubSystem,
                                         const SuperString&         nodeName,
                                         const Device::DeviceModel  deviceModel );
    void                createIoProcessorInt( const SuperString&                ioProcessorName,
                                              const std::vector<SuperString>&   channelModuleNames );
    void                createSubSystemInt( const SuperString&                  subSystemName,
                                            const Controller::ControllerType    controllerType,
                                            const std::vector<SuperString>&     controllerNames );
    Result              deserialize( const JSONObjectValue* const pObject );
    Result              deserializeDevices( const JSONArrayValue* const pArray,
                                            SubSystem* const            pSubSystem );
    Result              deserializeIoProcessor( const JSONObjectValue* const pObject );
    Result              deserializeIoProcessors( const JSONArrayValue* const pArray );
    Result              deserializeMount( const JSONObjectValue* const pObject );
    Result              deserializeMounts( const JSONArrayValue* const pArray );
    Result              deserializeSubSystem( const JSONObjectValue* const pObject );
    Result              deserializeSubSystems( const JSONArrayValue* const pArray );
    bool                isNodeNameUnique( const SuperString& name ) const;
    JSONObjectValue*    serializeDevice( const Device* const pDevice ) const;
    JSONObjectValue*    serializeIoProcessor( const IOProcessor* const pIoProcessor ) const;
    JSONArrayValue*     serializeIoProcessors() const;
    JSONArrayValue*     serializeMounts() const;
    JSONObjectValue*    serializeSubSystem( const SubSystem* const pSubSystem ) const;
    JSONArrayValue*     serializeSubSystems() const;

    static void         deserializeArrayToStrings( const JSONArrayValue* const      pArray,
                                                   std::vector<SuperString>* const  pOutput );

public:
    PersistableNodeTable()
    {
        m_IsUpdated = false;
    }

    ~PersistableNodeTable()
    {}

    Result              connectSubSystem( const SuperString&                subSystemName,
                                          const std::vector<SuperString>&   channelModuleNames );
    Result              createDevice( const SuperString&        subSystemName,
                                      const SuperString&        nodeName,
                                      const Device::DeviceModel deviceModel );
    Result              createIoProcessor();
    Result              createIoProcessor( const SuperString&               ioProcessorName,
                                           const std::vector<SuperString>&  channelModuleNames );
    Result              createSubSystem( const SuperString&                 subSystemName,
                                         const Controller::ControllerType   controllerType,
                                         const bool                         redundant );
    Result              createSubSystem( const SuperString&                 subSystemName,
                                         const Controller::ControllerType   controllerType,
                                         const std::vector<SuperString>&    controllerNames );
    Result              deleteDevice( const SuperString& deviceName );
    Result              deleteIoProcessor( const SuperString& iopName );
    Result              deleteSubSystem( const SuperString& subSystemName );
    Result              disconnectSubSystem( const SuperString& subSystemName );
    Result              load( const SuperString& fileName );
    Result              save( const SuperString& fileName );

    inline void         clear()
    {
        m_NodeSet.clear();
        m_IsUpdated = false;
    }

    //  Use these carefully - they may change value unexpectedly in multi-threaded contexts
    inline const IOPROCESSORS&  getIoProcessors() const     { return m_IoProcessors; }
    inline const SUBSYSTEMS&    getSubSystems() const       { return m_SubSystems; }
    inline bool                 isUpdated() const           { return m_IsUpdated; }

    static const char*          getResultString( const Result result );
};



#endif      /* EMSSP_PERSISTABLE_NODE_TABLE_H */

