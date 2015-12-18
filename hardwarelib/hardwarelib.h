//  hardwarelib header file
//  All hardwarelib clients should include this, and only this header file (from this library, I mean)



#ifndef     HARDWARELIB_H
#define     HARDWARELIB_H



#include    "../misclib/misclib.h"

#ifdef  WIN32
#include    "PSapi.h"
#endif



//  Following tags should be commented out to disable them
#define     EXECLIB_LOG_CHANNEL_IOS             0
#define     EXECLIB_LOG_CHANNEL_IO_ERRORS       1
#define     EXECLIB_LOG_CHANNEL_IO_BUFFERS      0   //  this won't always work if EXECLIB_LOG_CHANNEL_IOS isn't also def'd
#define     EXECLIB_LOG_DEVICE_IOS              0
#define     EXECLIB_LOG_DEVICE_IO_ERRORS        1
#define     EXECLIB_LOG_DEVICE_IO_BUFFERS       0   //  this won't always work if EXECLIB_LOG_DEVICE_IOS isn't also def'd



//  Describes a buffer for an IO - a single IO may use multiple buffers.
//  See IoAccessControlList.h
class   IoAccessControlWord
{
public:
    const ExecIoBufferAddressModifier   m_AddressModifier;
    Word36* const                       m_pBuffer;
    const COUNT                         m_BufferSize;

    IoAccessControlWord( Word36* const                          pBuffer,
                            const COUNT                         bufferSize,
                            const ExecIoBufferAddressModifier   addressModifier )
        :m_AddressModifier( addressModifier ),
        m_pBuffer( pBuffer ),
        m_BufferSize( bufferSize )
    {}

    inline Word36*      getWord( const INDEX wx ) const
    {
        if ( wx < m_BufferSize )
        {
            switch ( m_AddressModifier )
            {
            case EXIOBAM_DECREMENT:     return m_pBuffer - wx;
            case EXIOBAM_INCREMENT:     return m_pBuffer + wx;
            case EXIOBAM_NO_CHANGE:     return m_pBuffer;
            case EXIOBAM_SKIP_DATA:     return m_pBuffer;
            }
        }
        return 0;
    }

    //???? not sure why i need this but c++11 complains about delete operator== function...
    inline bool operator=( const IoAccessControlWord& comp ) const
    {
        return (m_AddressModifier == comp.m_AddressModifier)
                && (m_pBuffer == comp.m_pBuffer)
                && (m_BufferSize == comp.m_BufferSize);
    }
};



#include    "Node.h"
#include        "ChannelModule.h"
#include        "Controller.h"
#include            "DiskController.h"
#include            "TapeController.h"
#include        "Device.h"
#include            "DiskDevice.h"
#include                "FileSystemDiskDevice.h"
#include        "Processor.h"
#include            "IOProcessor.h"
#include    "IoAccessControlList.h"


#endif

