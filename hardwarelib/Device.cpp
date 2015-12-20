//  Device.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Device class definition



//  includes

#include    "hardwarelib.h"



// private / protected functions

//  ioEnd()
//
//  Derived classes must call this at the end of handling an io.
//
//  Parameters:
//      pIoInfo:			pointer to DevIoInfo object
void
Device::ioEnd
(
const IoInfo* const     pIoInfo
) const
{
#if EXECLIB_LOG_DEVICE_IO_ERRORS
    if (( pIoInfo->getStatus() != IoStatus::SUCCESSFUL )
        && ( pIoInfo->getStatus() != IoStatus::NO_INPUT ))
    {
        std::string logStr = "IoError Sts=";
        logStr += getIoStatusString( pIoInfo->getStatus(), pIoInfo->getSystemError() );
        logStr += " ";
        logStr += getIoInfoString( pIoInfo );
        writeLogEntry( logStr );
    }
#endif

#if EXECLIB_LOG_DEVICE_IO_BUFFERS
	if (( isReadFunction( pIoInfo->getFunction() ) )
		&& ( pIoInfo->getStatus() == IoStatus::SUCCESSFUL ))
	{
		writeBuffersToLog(pIoInfo);
	}
#endif
}


//  ioStart()
//
//  Derived classes must call this at the beginning of handling an io.
//  Performs any necessary logging, and posts an IoEvent to anyone listening.
//
//  Parameters:
//      pIoInfo:			pointer to DevIoInfo object
void
Device::ioStart
(
const IoInfo* const     pIoInfo
)
{
#if EXECLIB_LOG_DEVICE_IOS
    std::string logStr = "IoStart ";
    logStr += getIoInfoString( pIoInfo );
    writeLogEntry( logStr );
#endif

#if EXECLIB_LOG_DEVICE_IO_BUFFERS
    if ( isWriteFunction( pIoInfo->getFunction() ) )
        writeBuffersToLog( pIoInfo );
#endif
}



// constructors / destructors



//  public functions

//  dump()
void
Device::dump
(
std::ostream&       stream
) const
{
    Node::dump( stream );
    stream << "  Type: " << getDeviceTypeString( m_DeviceType )
            << " Model: " << getDeviceModelString( m_DeviceModel ) << std::endl;
    stream << "  Ready:" << (isReady() ? "YES" : "NO")
        << "  UnitAttn:" << (m_UnitAttentionFlag ? "YES" : "NO") << std::endl;
}


//  setReady()
//
//  Sets the device ready or not-ready, depending on the value of the Flag parameter.
//  Derivced classes should overload this as necessary, in order to do additional verification
//  to determine whether the state change should be allowed; however, upon deciding that
//  it is okay to proceed, they should call back to here.
//
//  Parameters:
//      flag:               true to set ready, else false
//
//  Returns:
//      true if successful, else false
bool
Device::setReady
(
const bool              flag
)
{
    m_ReadyFlag = flag;
    m_UnitAttentionFlag = flag;

    // notify the system log
    std::string logStr = "Device Ready ";
    logStr += (flag ? "Set" : "Cleared");
    writeLogEntry( logStr );

    return true;
}



//  static functions

//  getDeviceModelString())
//
//  Converts a DeviceModel value to a displayable string
//
//  Parameters:
//      model:              value to be converted
//
//  Returns:
//      string containing displayable text
const char*
Device::getDeviceModelString
(
const DeviceModel   model
)
{
    switch ( model )
    {
    case DeviceModel::FILE_SYSTEM_DISK:               return "FileSystemDisk";
    case DeviceModel::FILE_SYSTEM_PRINTER:            return "FileSystemPrinter";
    case DeviceModel::FILE_SYSTEM_PUNCH:              return "FileSystemPunch";
    case DeviceModel::FILE_SYSTEM_READER:             return "FileSystemReader";
    case DeviceModel::FILE_SYSTEM_TAPE:               return "FileSystemTape";
    }

    return "?";
}


//  getDeviceTypeString())
//
//  Converts a DeviceType value to a displayable string
//
//  Parameters:
//      type:               value to be converted
//
//  Returns:
//      string containing displayable text
const char*
Device::getDeviceTypeString
(
const Device::DeviceType        type
)
{
    switch ( type )
    {
    case DeviceType::DISK:              return "Disk";
    case DeviceType::SYMBIONT:          return "Symbiont";
    case DeviceType::TAPE:              return "Tape";
    }

    return "?";
}


//  getIoFunctionString()
//
//  Converts an IoFunction value to a displayable string
//
//  Parameters:
//      function:			value to be converted
//
//  Returns:
//      string containing displayable text
const char*
Device::getIoFunctionString
(
const IoFunction		function
)
{
    switch (function)
    {
    case IoFunction::CLOSE:                 return "Close";
    case IoFunction::DELETE_LINE:           return "Delete Line";
    case IoFunction::GET_INFO:              return "Get Info";
    case IoFunction::MOVE_BLOCK:            return "Move Block";
    case IoFunction::MOVE_BLOCK_BACKWARD:   return "Move Block Backward";
    case IoFunction::MOVE_FILE:             return "Move File";
    case IoFunction::MOVE_FILE_BACKWARD:    return "Move File Backward";
    case IoFunction::NONE:                  return "None";
    case IoFunction::READ:                  return "Read";
    case IoFunction::READ_BACKWARD:         return "Read Backward";
    case IoFunction::RESET:                 return "Reset";
    case IoFunction::REWIND:                return "Rewind";
    case IoFunction::REWIND_INTERLOCK:      return "Rewind Interlock";
    case IoFunction::SET_MODE:              return "Set Mode";
    case IoFunction::UNLOAD:                return "Unload";
    case IoFunction::WRITE:                 return "Write";
    case IoFunction::WRITE_END_OF_FILE:     return "Write End Of File";
    }

    return "???";
}


//  getIoInfoString()
//
//  Produces a string which compactly describes an IoInfo object
std::string
Device::getIoInfoString
(
const IoInfo* const     pIoInfo
)
{
    std::stringstream strm;
    if ( pIoInfo->getSource() )
        strm << "Src=" << pIoInfo->getSource()->getName();
    strm << " Fcn=" << getIoFunctionString( pIoInfo->getFunction() )
        << " BlkId=" << std::dec << pIoInfo->getBlockId()
        << " Count=" << pIoInfo->getByteCount()
        << " Xferd=" << pIoInfo->getBytesTransferred()
        << " Stat=" << getIoStatusString( pIoInfo->getStatus(), pIoInfo->getSystemError() );
    return strm.str();
}


//  getIoStatusString()
//
//  Converts an IoStatus (and optionally a system error code) into displayable text.
//
//  Parameters:
//      status:             IoStatus value to be converted
//      errorCode:          system error code, for IOST_SYSTEM_EXCEPTION
//
//  Returns:
//      string with displayable text
std::string
Device::getIoStatusString
(
const IoStatus          status,
const SYSTEMERRORCODE   errorCode
)
{
    std::string str = "";
    switch (status)
    {
    case IoStatus::SUCCESSFUL:              return "Successful";
    case IoStatus::BUFFER_TOO_SMALL:        return "Buffer Too Small";
    case IoStatus::DEVICE_BUSY:             return "Device Busy";
    case IoStatus::END_OF_TAPE:             return "End of Tape";
    case IoStatus::FILE_MARK:               return "File Mark";
    case IoStatus::INVALID_BLOCK_COUNT:     return "Invalid Block Count";
    case IoStatus::INVALID_BLOCK_ID:        return "Invalid Block Id";
    case IoStatus::INVALID_BLOCK_SIZE:      return "Invalid Block Size";
    case IoStatus::INVALID_DEVICE_ADDRESS:  return "Invalid Device Address";
    case IoStatus::INVALID_FUNCTION:        return "Invalid Function";
    case IoStatus::INVALID_LINE_NUMBER:     return "Invalid Line Number";
    case IoStatus::INVALID_MODE:            return "Invalid Mode";
    case IoStatus::INVALID_TRANSFER_SIZE:   return "Invalid Transfer Size";
    case IoStatus::IN_PROGRESS:             return "In Progress";
    case IoStatus::LOST_POSITION:           return "Lost Position";
    case IoStatus::MEDIA_ERROR:             return "Media Error";
    case IoStatus::NO_DEVICE:               return "No Device";
    case IoStatus::NO_INPUT:                return "No Input";
    case IoStatus::NOT_PREPPED:             return "Not Prepped";
    case IoStatus::NOT_READY:               return "Not Ready";
    case IoStatus::QUEUE_FULL:              return "Queue Full";
    case IoStatus::SYSTEM_EXCEPTION:
        str = "System Exception:";
        str += miscGetErrorCodeString( errorCode );
        return str;
    case IoStatus::UNIT_ATTENTION:          return "Unit Attention";
    case IoStatus::WRITE_PROTECTED:         return "Write Protected";
    }

    return "???";
}

