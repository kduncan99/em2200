//  Controller.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Header file for a generic virtual controller.
//  Superclassed from hwlib's Node, because we use hwlib Device's and they need Node's to talk to.



#ifndef     EXECLIB_CONTROLLER_H
#define     EXECLIB_CONTROLLER_H



#include    "Device.h"
#include    "Node.h"



class   Controller : public Node
{
public:
    enum class ControllerType
    {
        DISK,                   //  Disk Controller
        SYMBIONT,               //  Symbiont Controller
        TAPE,                   //  Tape Controller
    };


private:
    const ControllerType        m_ControllerType;

protected:
    Controller( const ControllerType    controllerType,
                const std::string&      name )
    :Node( Node::Category::CONTROLLER, name ),
            m_ControllerType( controllerType )
    {}

public:
    virtual ~Controller(){};

    inline ControllerType   getControllerType() const   { return m_ControllerType; }
    inline std::string      getControllerTypeString() const
    {
        return getControllerTypeString( m_ControllerType );
    }

    //  Disk devices require specific buffer sizes, matching the block size of the mounted pack.
    //  However, such block sizes are aligned on powers-of-two, and the IOs via C-format translation
    //  of 36-bit-world prep factors are aligned on however many bytes are required to hold
    //  prep-factor-aligned words.  For example, a 28-word IO in C-format, requires 126 bytes,
    //  where-as the corresponding physical block size would be 128 bytes.
    //
    //  Without some adjustment, all disk IOs would fail with invalid block size status.
    //
    //  It is therefore up to the channel module to create appropriately-sized buffers for device IO;
    //  however, the channel module does not know whether an IO is destined for a disk, a tape, or
    //  something else, so it cannot make this adjustment without help.
    //
    //  WE know whether our devices are tapes, disks, or whatever, and the ChannelModule knows which
    //  of us to ask, in order to set up the IO.  So, ChannelModules ask Controllers how big the
    //  device buffer should be, in order to support a given IO size.
    //
    //  TapeControllers should always respond with the exact input buffer size.
    //  DiskControllers should round-up to the next power-of-2.
    virtual COUNT               getContainingBufferSize( const COUNT requestedBufferSize ) const = 0;

    void                        routeIo( const NODE_ADDRESS     address,
                                         Device::IoInfo* const  pIoInfo );

    static const char*          getControllerTypeString( const ControllerType controllerType );
};



#endif
