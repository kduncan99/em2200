//  Implementation of PersistableNodeTable
//  Copyright (c) 2015 by Kurt Duncan
//
//  Loads the NodeTable from json-formatted flat file, saves it thereto.
//  Also notes disk mounts where appropriate, and saves those as well.


//  Format of saved data in the hardware config file:
//
//  Persisted JSON object:
//  { "ioProcessors": [ --ioProcessor--, ... ],
//    "subSystems": [ --subSystem--, ... ],
//    "mounts": [ --mount--, ... ] }
//
//  ioProcessor:
//  { "name": "IOP0",
//    "channelModuleNames": [ "CHM0-0", ... ] }
//
//  subSystem:
//  { "name": "Disk Subsystem",
//    "type": "DISK",
//    "controllerNames": [ "DSKCU0", "DISKCU1" ],
//    "channelModuleNames": [ "CHM0-0", "CHM1-0" ],
//    "devices:" [ --device--, ... ] }
//
//  device:
//  { "name": "DISK0",
//    "type": "Disk",
//    "model": "FileSystemDiskDevice" }
//
//  mount:
//  { "deviceName": "DISK0",
//    "mediaName": "dsk000" }



#include    "emssp.h"



#define     KEY_IOPROCESSOR_ARRAY       "ioProcessors"
#define     KEY_SUBSYSTEM_ARRAY         "subSystems"
#define     KEY_MOUNT_ARRAY             "mounts"



//  private methods

//  areNodeNamesUnique()
//
//  Tests a list of names to see if they are all unique
inline bool
PersistableNodeTable::areNodeNamesUnique
(
    const std::vector<SuperString>& names
) const
{
    for ( INDEX vx = 0; vx < names.size(); ++ vx)
    {
        if ( !isNodeNameUnique( names[vx] ) )
            return false;
    }

    return true;
}


//  createDeviceInt()
//
//  Creates a device of the given model, and adds it to the given subsystem.
//
//  Parameters:
//      pSubSystem:         pointer to the subsystem
//      nodeName:           name for the new device
//      deviceModel:        model of device
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::createDeviceInt
(
    SubSystem* const            pSubSystem,
    const SuperString&          nodeName,
    const Device::DeviceModel   deviceModel
)
{
    const std::list<Controller*>& controllers = pSubSystem->getControllers();
    Device* pDevice = 0;

    SuperString fixedDeviceName = nodeName;
    fixedDeviceName.foldToUpperCase();

    switch ( deviceModel )
    {
    case Device::DeviceModel::FILE_SYSTEM_DISK:
        pDevice = new FileSystemDiskDevice( fixedDeviceName );
        break;

#if 0 // TODO:SYM
    case Device::DeviceModel::FILE_SYSTEM_PRINTER:
        pDevice = new FileSystemPrinterDevice( fixedDeviceName );
        break;

    case Device::DeviceModel::FILE_SYSTEM_PRINTER:
        pDevice = new FileSystemPrinterDevice( fixedDeviceName );
        break;

    case Device::DeviceModel::FILE_SYSTEM_READER:
        pDevice = new FileSystemReaderDevice( fixedDeviceName );
        break;
#endif

#if 0 // TODO:TAPE
    case Device::DeviceModel::FILE_SYSTEM_TAPE:
        pDevice = new FileSystemTapeDevice( fixedDeviceName );
        break;
#endif

    default:
        return Result::TypeConflict;
    }

    //  Connect the device to the controller(s) in the subsystem
    for ( auto itctl = controllers.begin(); itctl != controllers.end(); ++itctl )
    {
        if ( !Node::connect( *itctl, pDevice ) )
            return Result::CannotConnect;
    }

    //  Finish up
    m_NodeSet.insert( pDevice );
    m_IsUpdated = true;
    return Result::Success;
}


//  createIoProcessorInt()
//
//  Creates an IOProcessor node with six channel modules and adds it to the node table.
//  Presumes all name-checking has been done.
void
PersistableNodeTable::createIoProcessorInt
(
    const SuperString&               ioProcessorName,
    const std::vector<SuperString>&  channelModuleNames
)
{
    IOProcessor* piop = new IOProcessor( ioProcessorName );
    m_IoProcessors[ioProcessorName] = piop;
    m_NodeSet.insert( piop );

    for ( INDEX cmx = 0; cmx < channelModuleNames.size(); ++cmx )
    {
        ChannelModule* pcm = new ChannelModule( channelModuleNames[cmx] );
        Node::connect( piop, cmx, pcm );
        pcm->startUp();
        m_NodeSet.insert( pcm );
    }

    m_IsUpdated = true;
}


//  createSubsystemInt()
//
//  Creates a subsystem which contains controllers of the indicated type.
//  The controller(s) in the subsystem will not be attached to any channel modules;
//  this must be done as a subsequent operation.
//
//  Parameters:
//      subSystemName:      Unique name for the subsystem
//      controllerType:     Type of controller to be supported
//      controllerNames:    Names of the controllers to be created
//
//  Returns:
//      Result enum indicating success or the reason for failure
void
PersistableNodeTable::createSubSystemInt
(
    const SuperString&                  subSystemName,
    const Controller::ControllerType    controllerType,
    const std::vector<SuperString>&     controllerNames
)
{
    std::list<Controller*> controllers;
    for ( INDEX cx = 0; cx < controllerNames.size(); ++cx )
    {
        Controller* pctl;
        switch ( controllerType )
        {
        case Controller::ControllerType::DISK:
            pctl = new DiskController( controllerNames[cx] );
            break;

        case Controller::ControllerType::SYMBIONT:
#if 0 //TODO:SYM
            pctl = new SymbiontController( controllerNames[cx] );
#endif
            break;

        case Controller::ControllerType::TAPE:
#if 0 //TODO:TAPE
            pctl = new TapeController( controllerNames[cx] );
#endif
            break;
        }

        controllers.push_back( pctl );
        m_NodeSet.insert( pctl );
    }

    SubSystem* pss = new SubSystem( subSystemName, controllers );
    m_SubSystems[subSystemName] = pss;

    m_IsUpdated = true;
}


//  deserialize()
//
//  Deserializes a JSONObjectValue loaded from a file which had been previously created by a call to store()
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserialize
(
    const JSONObjectValue* const    pObject
)
{
    JSONArrayValue* pIoProcessors = dynamic_cast<JSONArrayValue*>( pObject->getItem( KEY_IOPROCESSOR_ARRAY ) );
    JSONArrayValue* pSubSystems = dynamic_cast<JSONArrayValue*>(pObject->getItem( KEY_SUBSYSTEM_ARRAY ));
    JSONArrayValue* pMounts = dynamic_cast<JSONArrayValue*>(pObject->getItem( KEY_MOUNT_ARRAY ));
    if ( (pIoProcessors == 0) || (pSubSystems == 0) )
        return Result::ParseError;

    Result result = deserializeIoProcessors( pIoProcessors );
    if ( result != Result::Success )
        return result;

    result = deserializeSubSystems( pSubSystems );
    if ( result != Result::Success )
        return result;

    return deserializeMounts( pMounts );
}


//  deserializeDevices()
//
//  Takes a JSONArray describing a list of devices, deserializes them into Device objects,
//  and adds those objects to the indicated subsystem.
//
//  device:
//  { "name": "DISK0",
//    "type": "Disk",
//    "model": "FileSystemDiskDevice" }
//
//  Parameters:
//      pArray:         JSONArray
//      pSubSystem:     pointer to target subsystem
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeDevices
(
    const JSONArrayValue*       pArray,
    SubSystem* const            pSubSystem
)
{
    COUNT devices = pArray->getCount();
    for ( INDEX dx = 0; dx < devices; ++dx )
    {
        JSONObjectValue* pDevice = dynamic_cast<JSONObjectValue*>( pArray->getItem( dx ) );
        if ( pDevice == 0 )
            return Result::ParseError;

        JSONValue* pName = pDevice->getItem( "name" );
        JSONValue* pType = pDevice->getItem( "type" );
        JSONValue* pModel = pDevice->getItem( "model" );

        if ( (pName == 0) || (pType == 0) || (pModel == 0) )
            return Result::ParseError;

        SuperString name = pName->getValueAsString();
        SuperString type = pType->getValueAsString();
        SuperString model = pModel->getValueAsString();

        Device::DeviceModel deviceModel;
        if ( type.compareNoCase( "DISK" ) == 0 )
        {
            if ( type.compareNoCase( "FileSystemDisk" ) == 0 )
                deviceModel = Device::DeviceModel::FILE_SYSTEM_DISK;
            else
                return Result::TypeConflict;
        }
#if 0 // TODO:SYM
        else if ( type.compareNoCase( "SYMBIONT" ) == 0 )
        {
            if ( model.compareNoCase( "FileSystemPrinter" ) == 0 )
                deviceModel = Device::DeviceModel::FILE_SYSTEM_PRINTER;
            else if ( model.compareNoCase( "FileSystemReader" ) == 0 )
                deviceModel = Device::DeviceModel::FILE_SYSTEM_READER;
            else if ( model.compareNoCase( "FileSystemPunch" ) == 0 )
                deviceModel = Device::DeviceModel::FILE_SYSTEM_PUNCH;
            else
                return Result::TypeConflict;
        }
#endif
#if 0 // TODO:TAPE
        else if ( type.compareNoCase( "TAPE" ) == 0 )
        {
            if ( model.compareNoCase( "FileSystemTape" ) == 0 )
                deviceModel = Device::DeviceModel::FILE_SYSTEM_TAPE;
            else
                return Result::TypeConflict;
        }
#endif
        else
            return Result::ParseError;

        Result result = createDeviceInt( pSubSystem, name, deviceModel );
        if ( result != Result::Success )
            return result;
    }

    return Result::Success;
}


//  deserializeIoProcessor()
//
//  Deserializes an IO Processor, and adds the relevant objects to the NodeTable
//
//  ioProcessor:
//  { "name": "IOP0",
//    "channelModuleNames": [ "CHM0-0", ... ] }
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeIoProcessor
(
    const JSONObjectValue* const    pObject
)
{
    JSONStringValue* pIopName = dynamic_cast<JSONStringValue*>( pObject->getItem( "name" ) );
    JSONArrayValue* pCMNames = dynamic_cast<JSONArrayValue*>( pObject->getItem( "channelModuleNames" ) );
    if ( (pIopName == 0) || (pCMNames == 0) )
        return Result::ParseError;

    const SuperString& iopName = pIopName->getValueAsString();

    std::vector<SuperString> cmNames;
    deserializeArrayToStrings( pCMNames, &cmNames );

    return createIoProcessor( iopName, cmNames );
}


//  deserializeIoProcessors()
//
//  Deserializes a JSONArrayValue known to contain an array of serialized IoProcessor entries
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeIoProcessors
(
    const JSONArrayValue* const     pArray
)
{
    for ( INDEX ax = 0; ax < pArray->getCount(); ++ax )
    {
        JSONObjectValue* pObject = dynamic_cast<JSONObjectValue*>( pArray->getItem( ax ) );
        if ( pObject == 0 )
            return Result::ParseError;
        Result result = deserializeIoProcessor( pObject );
        if ( result != Result::Success )
            return result;
    }

    return Result::Success;
}


//  deserializeMount()
//
//  Given a JSON object describing a mount, we perform that mount
//
//  mount:
//  { "deviceName": "DISK0",
//    "mediaName": "dsk000" }
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeMount
(
    const JSONObjectValue* const pObject
)
{
    JSONStringValue* pDeviceName = reinterpret_cast<JSONStringValue*>( pObject->getItem( "deviceName" ) );
    JSONStringValue* pMediaName = reinterpret_cast<JSONStringValue*>( pObject->getItem( "mediaName" ) );
    if ( (pDeviceName == 0) || (pMediaName == 0) )
        return Result::ParseError;

    const SuperString deviceName = pDeviceName->getValueAsString();
    const SuperString mediaName = pMediaName->getValueAsString();

    for ( auto itn = m_NodeSet.begin(); itn != m_NodeSet.end(); ++itn )
    {
        if ( (*itn)->getName().compareNoCase( deviceName ) == 0 )
        {
            FileSystemDiskDevice* pDevice = dynamic_cast<FileSystemDiskDevice*>( *itn );
            if ( pDevice == 0 )
                return Result::TypeConflict;

            std::string fileName = PACKS_PATH + mediaName + ".dsk";
            if ( !pDevice->mount( fileName ) )
                return Result::MountFailed;
            return Result::Success;
        }
    }

    return Result::NodeDoesNotExist;
}


//  deserializeMounts()
//
//  Given a JSON array containing mount info, we deserialize them all, and perform the mount
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeMounts
(
    const JSONArrayValue* const  pArray
)
{
    bool mountFailed = false;

    for ( INDEX ax = 0; ax < pArray->getCount(); ++ax )
    {
        JSONObjectValue* pObject = dynamic_cast<JSONObjectValue*>( pArray->getItem( ax ) );
        if ( pObject == 0 )
            return Result::ParseError;
        Result result = deserializeMount( pObject );
        if ( result == Result::MountFailed )
            mountFailed = true;
        else if ( result != Result::Success )
            return result;
    }

    return mountFailed ? Result::MountFailed : Result::Success;
}


//  deserializeSubSystem()
//
//  Deserializes a SubSystem, and adds the relevant objects to the NodeTable
//
//  subSystem:
//  { "name": "Disk Subsystem",
//    "type": "DISK",
//    "controllerNames": [ "DSKCU0", "DISKCU1" ],
//    "channelModuleNames": [ "CHM0-0", "CHM1-0" ],
//    "devices:" [ --device--, ... ] }
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeSubSystem
(
    const JSONObjectValue* const    pObject
)
{
    JSONStringValue* pSubName = dynamic_cast<JSONStringValue*>( pObject->getItem( "name" ) );
    JSONStringValue* pSubType = dynamic_cast<JSONStringValue*>( pObject->getItem( "type" ) );
    JSONArrayValue* pCtlNames = dynamic_cast<JSONArrayValue*>( pObject->getItem( "controllerNames" ) );
    JSONArrayValue* pCMNames = dynamic_cast<JSONArrayValue*>( pObject->getItem( "channelModuleNames" ) );
    JSONArrayValue* pDevices = dynamic_cast<JSONArrayValue*>( pObject->getItem( "devices" ) );
    if ( (pSubName == 0) || (pSubType == 0) || (pCtlNames == 0) || (pCMNames == 0) || (pDevices == 0) )
        return Result::ParseError;

    const SuperString& subName = pSubName->getValueAsString();

    const SuperString& subTypeStr = pSubType->getValueAsString();
    Controller::ControllerType ctlType;
    if ( subTypeStr.compareNoCase("DISK") == 0 )
        ctlType = Controller::ControllerType::DISK;
#if 0 // TODO:SYM
    else if ( subTypeStr.compareNoCase("SYMBIONT") == 0 )
        ctlType = Controller::ControllerType::SYMBIONT;
#endif
#if 0 // TODO:TAPE
    else if ( subTypeStr.compareNoCase("TAPE") == 0 )
        ctlType = Controller::ControllerType::TAPE;
#endif
    else
        return Result::ParseError;

    std::vector<SuperString> ctlNames;
    deserializeArrayToStrings( pCtlNames, &ctlNames );

    Result result = createSubSystem( subName, ctlType, ctlNames );
    if ( result != Result::Success )
        return result;

    std::vector<SuperString> chmodNames;
    deserializeArrayToStrings( pCMNames, &chmodNames );

    result = connectSubSystem( subName, chmodNames );
    if ( result != Result::Success )
        return result;

    SubSystem* pSubSystem = m_SubSystems.find( subName )->second;
    result = deserializeDevices( pDevices, pSubSystem );
    if ( result != Result::Success )
        return result;

    return Result::Success;
}


//  deserializeSubSystems()
//
//  Deserializes a JSONArrayValue known to contain an array of serialized SubSystem entries
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deserializeSubSystems
(
    const JSONArrayValue* const     pArray
)
{
    for ( INDEX ax = 0; ax < pArray->getCount(); ++ax )
    {
        JSONObjectValue* pObject = dynamic_cast<JSONObjectValue*>( pArray->getItem( ax ) );
        if ( pObject == 0 )
            return Result::ParseError;
        Result result = deserializeSubSystem( pObject );
        if ( result != Result::Success )
            return result;
    }

    return Result::Success;
}


//  disconnectSubSystemInt()
//
//  Detaches all controllers in the subsystem from the various channel modules to which they are attached.
//
//  Parameters:
//      pSubSystemName:     Pointer to subsystem
//
//  Returns:
//      Result enum indicating success or the reason for failure
void
PersistableNodeTable::disconnectSubSystemInt
(
    SubSystem* const        pSubSystem
)
{
    std::list<Controller*> controllers = pSubSystem->getControllers();
    for ( auto itctl = controllers.begin(); itctl != controllers.end(); ++itctl )
    {
        Controller* pctl = *itctl;
        const Node::ANCESTORS& chmods = pctl->getAncestors();
        while ( !chmods.empty() )
        {
            Node* pcm = *(chmods.begin());
            Node::disconnect( pcm, pctl );
        }
    }
}


//  isNodeNameUnique()
//
//  Checks a proposed node name for uniqueness
//
//  Parameters:
//      name:           proposed name
//
//  Returns:
//      true if the name is unique
inline bool
PersistableNodeTable::isNodeNameUnique
(
    const SuperString&      name
) const
{
    for ( auto it = m_NodeSet.begin(); it != m_NodeSet.end(); ++it )
    {
        Node* pNode = *it;
        if ( pNode->getName().compareNoCase( name ) == 0 )
            return false;
    }

    return true;
}


//  serializeDevice()
//
//  Creates a JSON object containing a description of the indicated Device
//
//  format of serialized device object:
//  { "name": "DISK0",
//    "type": "Disk",
//    "model": "FileSystemDiskDevice" }
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONObjectValue*
PersistableNodeTable::serializeDevice
(
    const Device* const     pDevice
) const
{
    JSONObjectValue* pObject = new JSONObjectValue();
    pObject->store( "name", new JSONStringValue( pDevice->getName() ) );
    pObject->store( "type", new JSONStringValue( pDevice->getDeviceTypeString() ) );
    pObject->store( "model", new JSONStringValue( pDevice->getDeviceModelString() ) );
    return pObject;
}


//  serializeIoProcessor()
//
//  Creates a JSON object containing a description of the indicated IO processor
//
//  format of serialized ioProcessor object:
//  { "name": "IOP0",
//    "channelModuleNames": [ "CHM0-0", ... ] }
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONObjectValue*
PersistableNodeTable::serializeIoProcessor
(
    const IOProcessor* const    pIoProcessor
) const
{
    //  Need a JSON array to hold all the channel module names
    const Node::DESCENDANTS& cmNodes = pIoProcessor->getDescendants();
    JSONArrayValue* pCMNames = new JSONArrayValue();
    for ( auto itcm = cmNodes.begin(); itcm != cmNodes.end(); ++itcm )
    {
        const Node* pchmod = itcm->second;
        pCMNames->append( new JSONStringValue( pchmod->getName() ) );
    }

    //  Create the JSON object we're going to return
    JSONObjectValue* pObject = new JSONObjectValue();
    pObject->store( "name", new JSONStringValue( pIoProcessor->getName() ) );
    pObject->store( "channelModuleNames", pCMNames );

    return pObject;
}


//  serializeIoProcessors()
//
//  Create a JSON array containing JSON-serialization of all the IO processors we know about
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONArrayValue*
PersistableNodeTable::serializeIoProcessors() const
{
    JSONArrayValue* pArray = new JSONArrayValue();
    for ( auto itiop = m_IoProcessors.begin(); itiop != m_IoProcessors.end(); ++itiop )
        pArray->append( serializeIoProcessor( itiop->second ) );
    return pArray;
}


//  serializeMounts()
//
//  Walks the subsystem container looking for devices which can/should be auto-mounted,
//  building up a serialized JSONObject describing each, returning the lot in a JSONArray.
//
//  Format of serialized mount object:
//  { "deviceName": "DISK0",
//    "mediaName": "dsk000" }
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONArrayValue*
PersistableNodeTable::serializeMounts() const
{
    JSONArrayValue* pArray = new JSONArrayValue();

    for ( auto itss = m_SubSystems.begin(); itss != m_SubSystems.end(); ++itss )
    {
        const SubSystem* pSubSystem = itss->second;
        for ( auto itdev = pSubSystem->getDevices().begin(); itdev != pSubSystem->getDevices().end(); ++itdev )
        {
            const Device* pDevice = *itdev;
            if ( pDevice->getDeviceModel() == Device::DeviceModel::FILE_SYSTEM_DISK )
            {
                const FileSystemDiskDevice* pfsdd = dynamic_cast<const FileSystemDiskDevice*>( pDevice );

                JSONObjectValue* pMount = new JSONObjectValue();
                pMount->store( "deviceName", new JSONStringValue( pfsdd->getName() ) );
                pMount->store( "mediaName", new JSONStringValue( pfsdd->getPackName() ) );

                pArray->append( pMount );
            }
        }
    }

    return pArray;
}


//  serializeSubSystem()
//
//  Creates a JSON object containing a description of the indicated subsystem
//
//  Format of serialized subsystem object:
//  { "name": "Disk Subsystem",
//    "type": "DISK",
//    "controllerNames": [ "DSKCU0", "DISKCU1" ],
//    "channelModuleNames": [ "CHM0-0", "CHM1-0" ],
//    "devices:" [ --device--, ... ] }
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONObjectValue*
PersistableNodeTable::serializeSubSystem
(
    const SubSystem* const  pSubSystem
) const
{
    //  Serialize controller names
    JSONArrayValue* pCtlNames = new JSONArrayValue();
    const std::list<Controller*> controllers = pSubSystem->getControllers();
    for ( auto itctl = controllers.begin(); itctl != controllers.end(); ++itctl )
        pCtlNames->append( new JSONStringValue( (*itctl)->getName() ) );

    //  Serialize channel module names if we are connected.
    //  We only look at one controller; if there are two, connections should be identical.
    JSONArrayValue* pCmNames = new JSONArrayValue();
    Node::ANCESTORS chmods = pSubSystem->getControllers().front()->getAncestors();
    for ( auto itcm = chmods.begin(); itcm != chmods.end(); ++itcm )
        pCmNames->append( new JSONStringValue( (*itcm)->getName() ) );

    //  Serialize devices
    JSONArrayValue* pDevices = new JSONArrayValue();
    const std::list<Device*> devices = pSubSystem->getDevices();
    for ( auto itdev = devices.begin(); itdev != devices.end(); ++itdev )
        pDevices->append( serializeDevice( *itdev ) );

    JSONObjectValue* pObject = new JSONObjectValue();
    pObject->store( "name", new JSONStringValue( pSubSystem->getName() ) );
    pObject->store( "type", new JSONStringValue( pSubSystem->getControllers().front()->getControllerTypeString() ) );
    pObject->store( "controllerNames", pCtlNames );
    pObject->store( "channelModuleNames", pCmNames );
    pObject->store( "devices", pDevices );

    return pObject;
}


//  serializeSubSystems()
//
//  Create a JSON array containing JSON-serialization of all the subsystems we know about
//
//  Returns:
//      Result enum indicating success or the reason for failure
JSONArrayValue*
PersistableNodeTable::serializeSubSystems() const
{
    JSONArrayValue* pArray = new JSONArrayValue();
    for ( auto itss = m_SubSystems.begin(); itss != m_SubSystems.end(); ++itss )
        pArray->append( serializeSubSystem( itss->second ) );
    return pArray;
}



//  private static methods

inline void
PersistableNodeTable::deserializeArrayToStrings
(
    const JSONArrayValue* const     pArray,
    std::vector<SuperString>* const pOutput
)
{
    pOutput->resize( pArray->getCount() );
    for ( INDEX sx = 0; sx < pArray->getCount(); ++sx )
        (*pOutput)[sx] = pArray->getItem( sx )->getValueAsString();
}



//  public methods

//  connectSubSystem()
//
//  Connects the various controllers in the subsystem to the given channel module(s).
//  Each controller is connected to each channel module, so for a redundant subsystem (2 controllers)
//  with 2 channel modules specified, four connections are made.
//
//  Parameters:
//      subSystemName:      name of the subsystem to be connected
//      channelModuleNames: name(s) of the channel module(s) to which the controller(s) in the subsystem
//                              are to be connected.
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::connectSubSystem
(
    const SuperString&              subSystemName,
    const std::vector<SuperString>& channelModuleNames
)
{
    lock();

    //  Find the subsystem
    auto itss = m_SubSystems.find( subSystemName );
    if ( itss == m_SubSystems.end() )
    {
        unlock();
        return Result::NodeDoesNotExist;
    }

    SubSystem* pss = itss->second;

    //  Find channel modules
    std::set<ChannelModule*> channelModules;
    for ( auto itName = channelModuleNames.begin(); itName != channelModuleNames.end(); ++itName )
    {
        ChannelModule* pchmod = dynamic_cast<ChannelModule*>(getNode( *itName ));
        if ( pchmod == 0 )
        {
            unlock();
            return Result::NodeDoesNotExist;
        }

        channelModules.insert( pchmod );
    }

    //  Now register all controllers in the subsystem with all chosen channel modules.
    std::list<Controller*> controllers = pss->getControllers();
    for ( auto itctl = controllers.begin(); itctl != controllers.end(); ++itctl )
    {
        for ( auto itcm = channelModules.begin(); itcm != channelModules.end(); ++itcm )
        {
            if ( !Node::connect( *itcm, *itctl ) )
            {
                unlock();
                return Result::CannotConnect;
            }
        }
    }

    unlock();
    return Result::Success;
}


//  createDevice()
//
//  Creates a device of the given type and model, and adds it to the given subsystem.
//  Some sanity checks are done so that we don't, e.g., add a disk device to a tape subsystem.
//
//  Parameters:
//      subSystemName:      name of the subsystem to which the created device is to be added
//      deviceName:         name for the new device
//      deviceModel:        model of device
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::createDevice
(
    const SuperString&          subSystemName,
    const SuperString&          deviceName,
    const Device::DeviceModel   deviceModel
)
{
    lock();

    //  Is the device name unique?
    SuperString fixedDeviceName = deviceName;
    fixedDeviceName.foldToUpperCase();

    if ( (fixedDeviceName.substr(0, 3).compare("IOP") == 0)
          || (fixedDeviceName.substr(0, 3).compare("CHM") == 0)
          || (fixedDeviceName.substr(0, 5).compare("DSKCU") == 0)
          || (fixedDeviceName.substr(0, 5).compare("TAPCU") == 0)
          || (fixedDeviceName.substr(0, 5).compare("SYMCU") == 0) )
    {
        unlock();
        return Result::NameIsInvalid;
    }

    if ( !isNodeNameUnique( fixedDeviceName ) )
    {
        unlock();
        return Result::NameIsNotUnique;
    }

    //  Find the subsystem
    auto it = m_SubSystems.find( subSystemName );
    if ( it == m_SubSystems.end() )
    {
        unlock();
        return Result::NodeDoesNotExist;
    }

    SubSystem* pss = it->second;

    //  Do the work
    Result result = createDeviceInt( pss, fixedDeviceName, deviceModel );

    unlock();
    return result;
}


//  createIoProcessor()
//
//  Creates an IOProcessor node with six channel modules and adds it to the node table.
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::createIoProcessor()
{
    lock();

    //  Find an index number such that "IOP{n}" is a unique node name,
    //  as well as "CHM{n}-{x}" for {x} varying from '0' to '5'.
    INDEX iopIndex = 0;
    SuperString iopName;
    std::vector<SuperString> chmodNames;
    chmodNames.resize( 6 );

    do
    {
        std::stringstream iopStrm;
        iopStrm << "IOP" << iopIndex;
        iopName = iopStrm.str();

        for ( INDEX cmx = 0; cmx < 6; ++cmx )
        {
            std::stringstream cmStrm;
            cmStrm << "CHM" << iopIndex << "-" << cmx;
            chmodNames[cmx] = cmStrm.str();
        }

        ++iopIndex;
    } while ( !isNodeNameUnique( iopName ) || !areNodeNamesUnique( chmodNames ) );

    createIoProcessorInt( iopName, chmodNames );

    unlock();
    return Result::Success;
}


//  createIoProcessor()
//
//  Internal method to create an IO Processor with the given name,
//  along with ChannelModules subordinate thereto with the given names.
//
//  Parameters:
//      ioProcessorName:    Name to be assigned to the newly-created IOProcessor Node
//      channelModuleNames: Names to be assigned to the newly-created ChannelModule Nodes
//
//  Return:
//      Appropriate Result value
PersistableNodeTable::Result
PersistableNodeTable::createIoProcessor
(
    const SuperString&              ioProcessorName,
    const std::vector<SuperString>& channelModuleNames
)
{
    lock();
    if ( !isNodeNameUnique( ioProcessorName ) )
    {
        unlock();
        return Result::NameIsNotUnique;
    }

    if ( !miscIsValidNodeName( ioProcessorName ) )
    {
        unlock();
        return Result::NameIsInvalid;
    }

    for ( auto its = channelModuleNames.begin(); its != channelModuleNames.end(); ++its )
    {
        if ( !isNodeNameUnique( *its ) )
        {
            unlock();
            return Result::NameIsNotUnique;
        }

        if ( !miscIsValidNodeName( *its ) )
        {
            unlock();
            return Result::NameIsNotUnique;
        }
    }

    createIoProcessorInt( ioProcessorName, channelModuleNames );

    unlock();
    return Result::Success;
}


//  createSubsystem()
//
//  Creates a subsystem which contains 1 or 2 controllers of the indicated type.
//  The controller(s) in the subsystem will not be attached to any channel modules;
//  this must be done as a subsequent operation.
//
//  Parameters:
//      subSystemName:      Unique name for the subsystem
//      controllerType:     Type of controller to be supported
//      redunant:           true for 2 redundant controllers, false for 1 non-redundant
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::createSubSystem
(
    const SuperString&                  subSystemName,
    const Controller::ControllerType    controllerType,
    const bool                          redundant
)
{
    lock();

    //  Do case-insensitive name-uniqueness check
    SuperString chkName;
    for ( auto itss = m_SubSystems.begin(); itss != m_SubSystems.end(); ++itss )
    {
        if ( itss->second->getName().compareNoCase(subSystemName) == 0 )
        {
            unlock();
            return Result::NameIsNotUnique;
        }
    }

    //  Generate potential controller names, and iterate until they are unique
    std::string namePrefix;
    switch ( controllerType )
    {
    case Controller::ControllerType::DISK:
        namePrefix = "DSKCU";
        break;

    case Controller::ControllerType::SYMBIONT:
        namePrefix = "SYMCU";
        break;

    case Controller::ControllerType::TAPE:
        namePrefix = "TAPCU";
        break;
    }

    std::vector<SuperString> controllerNames;
    controllerNames.resize( redundant ? 2 : 1 );
    INDEX ctlIndex = 0;
    do
    {
        if ( ctlIndex > 9 )
        {
            unlock();
            return Result::GeneratedNameConflict;
        }

        std::stringstream strm0;
        strm0 << namePrefix << ctlIndex;
        controllerNames[0] = strm0.str();

        if ( redundant )
        {
            std::stringstream strm1;
            strm1 << namePrefix << (ctlIndex + 1);
            controllerNames[1] = strm1.str();
        }

        ctlIndex += 2;
    } while ( !areNodeNamesUnique( controllerNames ) );

    createSubSystemInt( subSystemName, controllerType, controllerNames );

    unlock();
    return Result::Success;
}


//  createSubsystem()
//
//  Creates a subsystem which contains controllers of the indicated type.
//  The controller(s) in the subsystem will not be attached to any channel modules;
//  this must be done as a subsequent operation.
//
//  Parameters:
//      subSystemName:      Unique name for the subsystem
//      controllerType:     Type of controller to be supported
//      controllerNames:    Names of the controllers to be created
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::createSubSystem
(
    const SuperString&                  subSystemName,
    const Controller::ControllerType    controllerType,
    const std::vector<SuperString>&     controllerNames
)
{
    lock();

    //  Do case-insensitive name-uniqueness check
    SuperString chkName;
    for ( auto itss = m_SubSystems.begin(); itss != m_SubSystems.end(); ++itss )
    {
        if ( itss->second->getName().compareNoCase(subSystemName) == 0 )
        {
            unlock();
            return Result::NameIsNotUnique;
        }
    }

    if ( !areNodeNamesUnique( controllerNames ) )
    {
        unlock();
        return Result::NameIsNotUnique;
    }

    createSubSystemInt( subSystemName, controllerType, controllerNames );

    unlock();
    return Result::Success;
}


//  deleteDevice()
//
//  Detaches a device from whichever subsystem contains it, then deletes it
//
//  Parameters:
//      deviceName:         name of the device to be deleted
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deleteDevice
(
    const SuperString&      deviceName
)
{
    lock();
    for ( auto itss = m_SubSystems.begin(); itss != m_SubSystems.end(); ++itss )
    {
        SubSystem* pss = itss->second;
        for ( auto itdev = pss->getDevices().begin(); itdev != pss->getDevices().end(); ++itdev )
        {
            Device* pdev = *itdev;
            if ( pdev->getName().compareNoCase( deviceName ) == 0 )
            {
                //  detach device from all controllers in the subsystem
                pss->removeDevice( pdev );

                //  delete the device
                m_NodeSet.erase( pdev );
                m_IsUpdated = true;
                unlock();
                return Result::Success;
            }
        }
    }

    unlock();
    return Result::NodeDoesNotExist;
}


//  deleteIoProcessor()
//
//  Deletes an IO Processor and its subordinate channel modules.
//  Any subsystems connected to deleted channel modules will become inaccessible,
//  and must be reconnected to some existing IO Processor (if any).
//
//  Parameters:
//      iopName:        name of IO Processor to be deleted
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deleteIoProcessor
(
    const SuperString&      iopName
)
{
    lock();

    SuperString chkName = iopName;
    chkName.foldToUpperCase();

    auto it = m_IoProcessors.find( chkName );
    if ( it == m_IoProcessors.end() )
    {
        unlock();
        return Result::NodeDoesNotExist;
    }

    //  Are any channel modules still connected to any controllers?
    IOProcessor* piop = it->second;
    Node::DESCENDANTS chmods = piop->getDescendants();
    for ( auto itcm = chmods.begin(); itcm != chmods.end(); ++itcm )
    {
        ChannelModule* pcm = dynamic_cast<ChannelModule*>(itcm->second);
        if ( pcm->getDescendants().size() > 0 )
        {
            unlock();
            return Result::ChannelModuleStillConnected;
        }
    }

    //  Delete channel modules (no need to deregister them), remove the IOP from our list, then delete it.
    for ( auto itcm = chmods.begin(); itcm != chmods.end(); ++itcm )
    {
        ChannelModule* pcm = dynamic_cast<ChannelModule*>(itcm->second);
        m_NodeSet.erase( pcm );
        delete pcm;
    }

    m_IoProcessors.erase( it );
    m_NodeSet.erase( piop );
    delete piop;

    m_IsUpdated = true;
    unlock();
    return Result::Success;
}


//  deleteSubSystem()
//
//  Deletes a subsystem including its contained controllers and devices,
//  after removing any references to said controllers from any channel modules.
//
//  Parameters:
//      subSystemName:  name of the subsystem to be deleted
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::deleteSubSystem
(
    const SuperString&  subSystemName
)
{
    lock();

    auto it = m_SubSystems.find( subSystemName );
    if ( it == m_SubSystems.end() )
    {
        unlock();
        return Result::NodeDoesNotExist;
    }

    SubSystem* pss = it->second;

    //  Detach controllers from channel modules
    disconnectSubSystemInt( pss );

    //  Delete controllers in the subsystem.
    //  Not necessary to remove them from the subsystem - it's going away pretty quickly.
    const std::list<Controller*> controllers = pss->getControllers();
    for ( auto itctl = controllers.begin(); itctl != controllers.end(); ++itctl )
    {
        Controller* pctl = *itctl;
        m_NodeSet.erase( pctl );
        delete pctl;
    }

    //  Delete devices in the subsystem.
    //  Not necessary to remove them from the subsystem.
    for ( auto itdev = pss->getDevices().begin(); itdev != pss->getDevices().end(); ++itdev )
    {
        Device* pdev = *itdev;
        m_NodeSet.erase( pdev );
        delete pdev;
    }

    //  Delete the subsystem
    m_SubSystems.erase( subSystemName );
    delete pss;

    m_IsUpdated = true;
    unlock();
    return Result::Success;
}


//  disconnectSubSystem()
//
//  Detaches all controllers in the subsystem from the various channel modules to which they are attached.
//
//  Parameters:
//      subSystemName:      Name of the subsystem to be detached
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::disconnectSubSystem
(
    const SuperString&      subSystemName
)
{
    lock();

    auto itss = m_SubSystems.find( subSystemName );
    if ( itss == m_SubSystems.end() )
    {
        unlock();
        return Result::NodeDoesNotExist;
    }

    SubSystem* pss = itss->second;
    disconnectSubSystemInt( pss );

    m_IsUpdated = true;
    unlock();
    return Result::Success;
}


//  load()
//
//  Clears, then loads the node table configuration from the indicated file in JSON format.
//
//  Parameters:
//      fileName:           name of the source file
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::load
(
    const SuperString&  fileName
)
{
    lock();
    clear();
    m_IsUpdated = false;

    SimpleFile file( fileName );
    SYSTEMERRORCODE errorCode = file.open( SimpleFile::READ );
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        unlock();
        return Result::FileOpenFailed;
    }

    COUNT64 fileSize;
    errorCode = file.getFileSize( &fileSize );
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        file.close();
        unlock();
        return Result::FileIoFailed;
    }

    COUNT64 bytes;
    BYTE* pBuffer = new BYTE[fileSize + 1];
    errorCode = file.read( 0, pBuffer, fileSize, &bytes );
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        delete[] pBuffer;
        file.close();
        unlock();
        return Result::FileIoFailed;
    }

    pBuffer[bytes] = ASCII_NUL;

    errorCode = file.close();
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        delete[] pBuffer;
        unlock();
        return Result::FileCloseFailed;
    }

    JSONParser parser;
    std::string str;
    str.append( reinterpret_cast<char *>(pBuffer) );
    delete[] pBuffer;

    //  Decode main JSON objects
    JSONValue* pValue = 0;
    try
    {
        pValue = parser.parse( str );
    }
    catch (JSONException* pe)
    {
        unlock();
        return Result::ParseError;
    }

    JSONObjectValue* pObject = dynamic_cast<JSONObjectValue*>( pValue );
    if ( pObject == 0 )
    {
        unlock();
        return Result::ParseError;
    }

    Result result = deserialize( pObject );
    unlock();
    return result;
}


//  save()
//
//  Saves the node table configuration in JSON format in the indicated file.
//
//  Parameters:
//      fileName:           name of the destination file
//
//  Returns:
//      Result enum indicating success or the reason for failure
PersistableNodeTable::Result
PersistableNodeTable::save
(
    const SuperString&  fileName
)
{
    lock();

    //  Build JSON object
    JSONObjectValue* pSaveObject = new JSONObjectValue();
    pSaveObject->store( KEY_IOPROCESSOR_ARRAY, serializeIoProcessors() );
    pSaveObject->store( KEY_SUBSYSTEM_ARRAY, serializeSubSystems() );
    pSaveObject->store( KEY_MOUNT_ARRAY, serializeMounts() );

    //  Serialize the JSON object then discard it
    std::string data = pSaveObject->encode();
    delete pSaveObject;

    SimpleFile file( fileName );
    SYSTEMERRORCODE errorCode = file.open( SimpleFile::WRITE | SimpleFile::TRUNCATE );
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        unlock();
        return Result::FileOpenFailed;
    }

    COUNT bytes;
    errorCode = file.write( 0, reinterpret_cast<const BYTE*>(data.c_str()), data.size(), &bytes );
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        file.close();
        unlock();
        return Result::FileIoFailed;
    }

    errorCode = file.close();
    if ( errorCode != SYSTEMERRORCODE_SUCCESS )
    {
        unlock();
        return Result::FileCloseFailed;
    }

    m_IsUpdated = false;
    unlock();
    return Result::Success;
}



//  public static methods

//  getResultString()
//
//  Converts a Result value to something displayable
const char*
PersistableNodeTable::getResultString
(
    const Result    result
)
{
    switch ( result )
    {
    case Result::CannotConnect:                 return "The subsystem cannot be connected";
    case Result::ChannelModuleStillConnected:   return "At least one channel module is still connected to a controller";
    case Result::FileCloseFailed:               return "File close failed";
    case Result::FileIoFailed:                  return "File IO failed";
    case Result::FileOpenFailed:                return "File open failed";
    case Result::GeneratedNameConflict:         return "There is a conflict in automatically generated names";
    case Result::MountFailed:                   return "Mount Failed";
    case Result::NameIsInvalid:                 return "A proposed name is invalid";
    case Result::NameIsNotUnique:               return "A proposed name is not unique";
    case Result::NodeDoesNotExist:              return "A requested entity does not exist for the given name";
    case Result::ParseError:                    return "Parse error occurred while loading the node table";
    case Result::Success:                       return "Success";
    case Result::TypeConflict:                  return "Type conflict";
    }

    return "???";
}