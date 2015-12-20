//	DiskDevice.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	Base class descriptor of all emulated (and actual) disk nodes
// 
//	All transfers are 8-bit bytes.  Word data should be packed 72-bits to 9-bytes.



#ifndef     HARDWARELIB_DISK_DEVICE_H
#define     HARDWARELIB_DISK_DEVICE_H



//  includes

#include    "Device.h"



//  constants



//  macros



//  enums



//  typedefs



//  class declarations

class   DiskDevice : public Device
{
public:
    struct DiskDeviceInfo
    {
        //  Superclass of basic DeviceInfo class, describing this device's characteristics --
        //  Returned in the caller's buffer for an IOF_GET_INFO function.
        //  There is some trickery involved, as the ultimate caller is probably living in 36-bit space...
        //  It's up to the caller to decide whether to ask for this in xlat mode A or C.
        DeviceInfo              m_DeviceInfo;
        BLOCK_COUNT             m_BlockCount;
        BLOCK_SIZE              m_BlockSize;
        bool                    m_IsMounted;
        bool                    m_IsReady;
        bool                    m_IsWriteProtected;
    };

private:
    BLOCK_COUNT             m_BlockCount;           //  number of blocks on the mounted pack
    BLOCK_SIZE              m_BlockSize;            //  block size (in bytes) of the mounted pack
    bool                    m_IsMounted;
    bool                    m_IsWriteProtected;

    void                    getDeviceInfo( struct DiskDeviceInfo* const pInfo );

protected:
    inline void setIsMounted( const bool flag )
    {
        m_IsMounted = flag;
    }

    inline void setBlockCount( const BLOCK_COUNT blocks )
    {
        m_BlockCount = blocks;
    }

    inline void setBlockSize( const BLOCK_SIZE size )
    {
        m_BlockSize = size;
    }

    // Device interface
    void                    ioGetInfo( IoInfo* const );
    virtual void            writeBuffersToLog( const IoInfo* const ) const;

    //  abstract virtuals
    virtual void            ioRead( IoInfo* const ) = 0;
    virtual void            ioReset( IoInfo* const ) = 0;
    virtual void            ioUnload( IoInfo* const ) = 0;
    virtual void            ioWrite( IoInfo* const ) = 0;

    DiskDevice
    (
    const DeviceModel   model,
    const std::string&  name
    );

public:
    virtual ~DiskDevice(){}

    bool                    setIsWriteProtected( const bool flag );

    // Device interface
    void                    handleIo( IoInfo* const pIoInfo );
    virtual bool            setReady( const bool flag );

    //  Node interface
    virtual void            dump( std::ostream& stream ) const = 0;

    //  abstract virtuals

	//  inlines
    inline BLOCK_COUNT      getBlockCount() const           { return m_BlockCount; }
    inline BLOCK_SIZE       getBlockSize() const            { return m_BlockSize; }
    inline bool             isMounted() const               { return m_IsMounted; }
    inline bool             isPrepped() const               { return m_BlockSize != 0; }
    inline bool             isWriteProtected() const        { return m_IsWriteProtected; }
};



#endif
