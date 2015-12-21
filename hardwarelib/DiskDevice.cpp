//  DiskDevice.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  DiskDevice abstract base class definition



//  includes

#include	"hardwarelib.h"



//  constants



//  macros



//  enumerators



//  typedefs



//  structs



//  internal function prototypes



//  statics



//  internal functions



//  private / protected functions

//  ioGetInfo()
//
//  io handler for Device::IoFunction::GET_INFO
//
//  This is an immediate IO, no waiting.
void
DiskDevice::ioGetInfo
(
IoInfo* const		pIoInfo
)
{
    DiskDeviceInfo* pInfo = reinterpret_cast<DiskDeviceInfo*>( pIoInfo->getBuffer() );

    if ( pIoInfo->getByteCount() < sizeof(DiskDeviceInfo) )
    {
        pIoInfo->setStatus( IoStatus::INVALID_BLOCK_SIZE );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    //  Clear UA flag - user is asking for the info pending for him.
    m_UnitAttentionFlag = false;

    pInfo->m_DeviceInfo.m_DeviceModel = getDeviceModel();
    pInfo->m_DeviceInfo.m_DeviceType = getDeviceType();
    pInfo->m_BlockCount = getBlockCount();
    pInfo->m_BlockSize = getBlockSize();
    pInfo->m_IsMounted = isMounted();
    pInfo->m_IsReady = isReady();
    pInfo->m_IsWriteProtected = isWriteProtected();

    pIoInfo->setBytesTransferred( sizeof(DiskDeviceInfo) );
    pIoInfo->setStatus( IoStatus::SUCCESSFUL );
    if ( pIoInfo->getSource() )
        pIoInfo->getSource()->signal( this );
}


//  writeBuffersToLog()
//
//  Writes the current read or write buffer to the system log.
//  Don't do this unless you really need to.
void
DiskDevice::writeBuffersToLog
(
const IoInfo*       pIoInfo
) const
{
    const IoInfo* pDiskIoInfo = dynamic_cast<const IoInfo*>(pIoInfo);
    if ( pDiskIoInfo && pDiskIoInfo->getBuffer() )
        miscWriteBufferToLog( getName(), "IO Buffer", pDiskIoInfo->getBuffer(), getBlockSize() );
}



// constructors / destructors

DiskDevice::DiskDevice
(
const DeviceModel       model,
const std::string&      name
)
:Device( Device::DeviceType::DISK, model, name ),
        m_BlockCount( 0 ),
        m_BlockSize( 0 ),
        m_IsMounted( false ),
        m_IsWriteProtected( true )
{
}



// public functions

//  dump()
void
DiskDevice::dump
(
std::ostream&       stream
) const
{
    Device::dump( stream );
    stream << "  Block Size:      " << m_BlockSize << std::endl;
    stream << "  Block Count:     " << m_BlockCount << std::endl;
    stream << "  Mounted:         " << (m_IsMounted ? "YES" : "NO") << std::endl;
    stream << "  Write Protected: " << (m_IsWriteProtected ? "YES" : "NO") << std::endl;
}


//  handleIo()
//
//  Calls the appropriate local or virtual function based on the information in the
//  DiskIoInfo object.
//
//  Parameters:
//      pIoInfo:            pointer to DiskIoInfo object
void
DiskDevice::handleIo
(
IoInfo*             pIoInfo
)
{
    IoInfo* pDiskIoInfo = dynamic_cast<IoInfo*>(pIoInfo);
    assert(pDiskIoInfo);

    lock();
    ioStart( pDiskIoInfo );

    switch ( pDiskIoInfo->getFunction() )
    {
    case Device::IoFunction::GET_INFO:
        ioGetInfo( pDiskIoInfo );
        break;

    case Device::IoFunction::READ:
        ioRead( pDiskIoInfo );
        break;

    case Device::IoFunction::RESET:
        ioReset( pDiskIoInfo );
        break;

    case Device::IoFunction::UNLOAD:
        ioUnload( pDiskIoInfo );
        break;

    case Device::IoFunction::WRITE:
        ioWrite( pDiskIoInfo );
        break;

    default:
        pDiskIoInfo->setStatus( Device::IoStatus::INVALID_FUNCTION );
        if ( pDiskIoInfo->getSource() )
            pDiskIoInfo->getSource()->signal( this );
    }

    ioEnd( pDiskIoInfo );
    unlock();
}


//  setIsWriteProtected()
//
//  Sets write protected, or clears it if it is mounted.
//
//  Parameters:
//      flag:           true to set write protected, false otherwise
//
//  Returns:
//      true if state change successful, else false
bool
DiskDevice::setIsWriteProtected
(
    const bool          flag
)
{
    lock();

    if ( !flag && !isMounted() )
    {
        unlock();
        return false;
    }

    m_IsWriteProtected = flag;

    std::stringstream logStream;
    logStream << "Write Protect " << (flag ? "Set" : "Cleared");
    writeLogEntry( logStream.str() );

    unlock();
    return true;
}


//  setReady()
//
//  Makes sure the device can be set ready, then calls the base class to do that
//
//  Parameters:
//      flag:           true to set ready, false otherwise
//
//  Returns:
//      true if state change successful, else false
bool
DiskDevice::setReady
(
const bool			flag
)
{
    lock();
    if (flag && !isMounted())
    {
        unlock();
        return false;
    }

    bool retn = Device::setReady( flag );

    unlock();
    return retn;
}

