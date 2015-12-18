//  FileSystemDiskDevice.cpp
//
//  FileSystemDiskDevice class definition - emulates a disk device in a system file.
//
//  Filesystem-hosted disk packs are stored as logical blocks of fixed size, according to
//  the prep factor specified when the disk pack is prepped.  Sizes are as follows:
//      PrepFactor  BlockSize
//      in Words:   in Bytes:
//          28          128
//          56          256
//         112          512
//         224         1024
//         448         2048
//         896         4096     (non-standard, but we accept it due to NTFS block sizing)
//        1792         8192
//
//  Device-relative logical block numbers start at 0, and are mapped to physical byte
//  addresses according to the following algorithm:
//      physical_byte_addr = (logical_block_number + 1) * block_size
//  This leaves us with a scratch-pad area in the first logical block.
//  The scratch pad exists so that we can identify the logical geometry of the data in the
//  hosted file without having to know the geometry beforehand.
//
//  We should note that, in observance of emulated hardware conventions, we do not recognize
//  or care about the Word36 world.  Some entity in the stack above us is responsible for
//  conversions between the Word36 world and the byte world.  Thus, we actually have no
//  visibility to prep factors.  Specifically, we do not have any idea how 36-bit data is
//  packaged into the logical blocks, and even the conversion table above, has no relevance
//  to the code here-in.
//
//  The format of the scratchpad area is as such (all integer values are big-endian)
//      byte    byte
//     offset  length   description
//        0       8     scratchpad identification "EM2200" {0} {0}
//        8       4     scratchpad major version (differing major versions are incompatible)
//       12       4     scratchpad minor version (differing minor versions are compatible)
//       16       4     logical block size
//       20       8     logical block count



//  includes

#include    "hardwarelib.h"



// constants

#define     INFO_ID                 "EM2200"
#define     INFO_MAJOR_VERSION      1
#define     INFO_MINOR_VERSION      1



//  private / protected functions

//  calculateByteOffset()
//
//  Single location for converting a logical block ID to a physical byte offset.
//  Caller must verify blockId is within the blockcount range.
inline COUNT64
FileSystemDiskDevice::calculateByteOffset
(
const BLOCK_ID      blockId
) const
{
    return (blockId + 1) * getBlockSize();
}


//  ioRead()
//
//  Reads logical records from the underlying data store.
//
//  Parameters:
//      pIoInfo:            pointer to DiskIoInfo object
void
FileSystemDiskDevice::ioRead
(
IoInfo* const       pIoInfo
)
{
    if ( !isReady() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_READY );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( m_UnitAttentionFlag )
    {
        pIoInfo->setStatus( Device::IoStatus::UNIT_ATTENTION );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( !isPrepped() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_PREPPED );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    COUNT reqByteCount = pIoInfo->getByteCount();
    if ( (reqByteCount == 0) || ((reqByteCount % getBlockSize()) != 0) )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_SIZE );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    BLOCK_ID reqBlockId = pIoInfo->getBlockId();
    BLOCK_COUNT reqBlockCount = reqByteCount / getBlockSize();

    if ( reqBlockId >= getBlockCount() )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_ID );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( (reqBlockId + reqBlockCount > getBlockCount()) || (reqByteCount > 0x7FFFFFFF) )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_COUNT );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    pIoInfo->setStatus( Device::IoStatus::IN_PROGRESS );
    COUNT64 byteOffset = calculateByteOffset( reqBlockId );

    COUNT bytesRead;
    SYSTEMERRORCODE result = m_pSimpleFile->read( byteOffset, pIoInfo->getBuffer(), reqByteCount, &bytesRead );

    pIoInfo->setBytesTransferred( bytesRead );
    pIoInfo->setSystemError( result );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        pIoInfo->setStatus( Device::IoStatus::SYSTEM_EXCEPTION );
        writeLogEntry( "Read failed on " +  m_pSimpleFile->getFileName() + ":" + miscGetErrorCodeString( result ));
    }
    else
        pIoInfo->setStatus( Device::IoStatus::SUCCESSFUL );

    if ( pIoInfo->getSource() )
        pIoInfo->getSource()->signal( this );
}


//  ioReset()
//
//  Resets the device - not much to do
//
//  Parameters:
//      pIoInfo:            pointer to DiskIoInfo object
void
FileSystemDiskDevice::ioReset
(
IoInfo* const       pIoInfo
)
{
    if ( !isReady() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_READY );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    pIoInfo->setStatus( Device::IoStatus::SUCCESSFUL );
    if ( pIoInfo->getSource() )
        pIoInfo->getSource()->signal( this );
}


//  ioUnload()
//
//  Unmounts the current media, leaving the device not-ready.
//  Generally, we cannot be ready if we are not mounted, so we expect the
//  UnMount() below to succeed, without actually checking for a mounted file stream.
//
//  Parameters:
//      pIoInfo:            pointer to DiskIoInfo object
void
FileSystemDiskDevice::ioUnload
(
IoInfo* const       pIoInfo
)
{
    if ( !isReady() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_READY );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    unmount();
    pIoInfo->setStatus( Device::IoStatus::SUCCESSFUL );
    if ( pIoInfo->getSource() )
        pIoInfo->getSource()->signal( this );
    return;
}


//  ioWrite()
//
//  Writes a data block to the media, at the current host system file pointer.
//
//
//  Parameters:
//      pIoInfo:            pointer to DiskIoInfo object
void
FileSystemDiskDevice::ioWrite
(
IoInfo* const       pIoInfo
)
{
    if ( !isReady() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_READY );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( m_UnitAttentionFlag )
    {
        pIoInfo->setStatus( Device::IoStatus::UNIT_ATTENTION );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( !isPrepped() )
    {
        pIoInfo->setStatus( Device::IoStatus::NOT_PREPPED );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( isWriteProtected() )
    {
        pIoInfo->setStatus( Device::IoStatus::WRITE_PROTECTED );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    COUNT reqByteCount = pIoInfo->getByteCount();
    if ( (reqByteCount == 0) || ((reqByteCount % getBlockSize()) != 0) )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_SIZE );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    BLOCK_ID reqBlockId = pIoInfo->getBlockId();
    BLOCK_COUNT reqBlockCount = reqByteCount / getBlockSize();

    if ( reqBlockId >= getBlockCount() )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_ID );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    if ( (reqBlockId + reqBlockCount > getBlockCount()) || (reqByteCount > 0x7FFFFFFF) )
    {
        pIoInfo->setStatus( Device::IoStatus::INVALID_BLOCK_COUNT );
        if ( pIoInfo->getSource() )
            pIoInfo->getSource()->signal( this );
        return;
    }

    pIoInfo->setStatus( Device::IoStatus::IN_PROGRESS );

    pIoInfo->setStatus( Device::IoStatus::IN_PROGRESS );
    COUNT64 byteOffset = calculateByteOffset( reqBlockId );

    COUNT bytesWritten;
    SYSTEMERRORCODE result = m_pSimpleFile->write( byteOffset, pIoInfo->getBuffer(), reqByteCount, &bytesWritten );

    pIoInfo->setBytesTransferred( bytesWritten );
    pIoInfo->setSystemError( result );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        pIoInfo->setStatus( Device::IoStatus::SYSTEM_EXCEPTION );
        writeLogEntry( "Write failed on " +  m_pSimpleFile->getFileName() + ":" + miscGetErrorCodeString( result ));
    }
    else
        pIoInfo->setStatus( Device::IoStatus::SUCCESSFUL );

    if ( pIoInfo->getSource() )
        pIoInfo->getSource()->signal( this );
}


//  readScratchPadInfo()
//
//  During mount process, we need to determine disk geometry.
//  This does that.
bool
FileSystemDiskDevice::readScratchPadInfo
(
ScratchPadInfo* const   pInfo
) const
{
    ScratchPad scratchPad;
    COUNT xferBytes;
    SYSTEMERRORCODE result = m_pSimpleFile->read( 0,
                                                  reinterpret_cast<BYTE*>(&scratchPad),
                                                  sizeof(ScratchPad),
                                                  &xferBytes );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        writeLogEntry( "Cannot get disk geometry" );
        return false;
    }

    deserializeScratchPad( pInfo, &scratchPad );
    return true;
}


//  writeScratchPadInfo()
//
//  Writes scratch pad to the media.
bool
FileSystemDiskDevice::writeScratchPadInfo
(
const ScratchPadInfo&   info
) const
{
    ScratchPad scratchPad;
    serializeScratchPad( &scratchPad, &info );

    COUNT xferBytes;
    SYSTEMERRORCODE result = m_pSimpleFile->write( 0,
                                                   reinterpret_cast<BYTE*>(&scratchPad),
                                                   sizeof(ScratchPad),
                                                   &xferBytes );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        writeLogEntry( "Cannot write disk geometry" );
        return false;
    }

    return true;
}



// constructors / destructors

FileSystemDiskDevice::~FileSystemDiskDevice()
{
    if ( isMounted() )
        unmount();
}



//  public functions

//  dump()
void
FileSystemDiskDevice::dump
(
std::ostream&       stream
) const
{
    DiskDevice::dump( stream );

    if ( isMounted() )
        stream << "  File Name:     " << m_pSimpleFile->getFileName() << std::endl;
}


//  mount()
//
//  Mounts the media for this device.
//  For a FileSystemDiskDevice, this entails opening a file-system file.
//
//  Parameters:
//      mediaName:          string containing file name to be mounted
//
//  Returns:
//      true if successful, else false
bool
FileSystemDiskDevice::mount
(
const std::string&      mediaName
)
{
    if ( isMounted() )
    {
        std::stringstream logStream;
        logStream << "Cannot mount " << mediaName << ":Already mounted";
        writeLogEntry( logStream.str() );
        return false;
    }

    //  Open the file
    m_pSimpleFile = new SimpleFile( mediaName );
    unsigned int flags = SimpleFile::READ | SimpleFile::WRITE | SimpleFile::EXISTING;
    SYSTEMERRORCODE result = m_pSimpleFile->open( flags );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        writeLogEntry( "Open failed on " + mediaName + ":" + miscGetErrorCodeString( result ));
        delete m_pSimpleFile;
        return false;
    }

    setIsMounted( true );

    //  Read scratch pad to determine pack geometry
    ScratchPadInfo info;
    readScratchPadInfo( &info );
    if ( (info.m_Identifier.compare(INFO_ID) != 0)
        || (info.m_MajorVersion != INFO_MAJOR_VERSION) )
    {
        std::stringstream logStream;
        logStream << "During mount of " << mediaName << ":Bad scratchpad ID or major version number";
        writeLogEntry( logStream.str() );
        setBlockSize( 0 );
        setBlockCount( 0 );

    }
    else
    {
        setBlockSize( info.m_BlockSize );
        setBlockCount( info.m_BlockCount );
    }

    return true;
}


//  setReady()
//
//  Overrides simple call to set the ready flag.  Prevents setting true if we're not mounted.
bool
FileSystemDiskDevice::setReady
(
const bool          readyFlag
)
{
    if ( readyFlag == isReady() )
        return true;

    if ( readyFlag && !isMounted() )
        return false;
    return DiskDevice::setReady( readyFlag );
}


//  unmount()
//
//  Unmounts the currently-mounted media
//
//  Returns:
//      true if successful, else false
bool
FileSystemDiskDevice::unmount()
{
    if ( !isMounted() )
        return false;

    //  Clear ready flag to prevent any more IOs from coming in.
    setReady( false );

    // Release the host system file
    m_pSimpleFile->close();
    delete m_pSimpleFile;
    m_pSimpleFile = 0;

    setBlockCount( 0 );
    setBlockSize( 0 );
    setIsMounted( false );
    setIsWriteProtected( true );

    return true;
}



//  static function

//  createPack()
//
//  Creates a host system file which can be mounted on this type of device as a pack
bool
FileSystemDiskDevice::createPack
(
const std::string&      fileName,
const BLOCK_SIZE        blockSize,
const BLOCK_COUNT       blockCount,
std::string* const      pErrorMessage
)
{
    if ( !miscIsValidBlockSize( blockSize ) )
    {
        *pErrorMessage = "Invalid block size";
        return false;
    }

    TRACK_COUNT trackCount = (miscGetPrepFactorFromBlockSize( blockSize ) * blockCount) / 1792;
    if ( (trackCount < 10000) || (trackCount > 99999) )
    {
        *pErrorMessage = "Invalid track count";
        return false;
    }

    //  Create system file to contain the pack image
    SimpleFile simpleFile( fileName );
    unsigned int flags = SimpleFile::NEW | SimpleFile::WRITE;
    SYSTEMERRORCODE result = simpleFile.open( flags );

    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        *pErrorMessage = "Open failed on " +  simpleFile.getFileName() + ":" + miscGetErrorCodeString( result );
        return false;
    }

    //  (re)-write scratchpad area.
    ScratchPadInfo info;
    info.m_Identifier = INFO_ID;
    info.m_MajorVersion = INFO_MAJOR_VERSION;
    info.m_MinorVersion = INFO_MINOR_VERSION;
    info.m_BlockCount = blockCount;
    info.m_BlockSize = blockSize;

    ScratchPad scratchPad;
    serializeScratchPad( &scratchPad, &info );

    COUNT xferBytes;
    simpleFile.write( 0, reinterpret_cast<BYTE*>(&scratchPad), sizeof(ScratchPad), &xferBytes );
    if ( result != SYSTEMERRORCODE_SUCCESS )
    {
        *pErrorMessage = "Write failed on " +  simpleFile.getFileName() + ":" + miscGetErrorCodeString( result );
        simpleFile.close();
        return false;
    }

    simpleFile.close();
    return true;
}

