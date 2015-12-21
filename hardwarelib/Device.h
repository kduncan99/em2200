//	Device.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Device base class declaration
//  A Device is the lowest unit in an IO chain.
//  Generally, Devices are child Nodes for Controllers.
//  All IO to Devices is done asynchronously.
//      The Controller sends a DeviceIoInfo to the Device node via a pointer.
//      If the IO cannot be scheduled, the IoStatus field of the DeviceIoInfo is updated immediately, and the IO is rejected.
//      For IOs which are asynchronous at the system level, the IO is performed, IoStatus is updated,
//          a signal is sent to the sender, and the IO is considered complete.
//      For IOs which are synchronous at the system level, the IO is scheduled, IoStatus is updated, and the IO is accepted.
//          When the IO completes at the system level, a signal is sent to the sender.
//      Senders must preserve DeviceIoInfo objects in memory until the Device marks the IoStatus field with some
//          value indicating that the IO is complete, rejected, or has failed.
//      Actual system IO may involve invoking the host system's asynchronous IO facility.  For Windows, this is accomplished
//          via ReadFileEx() and WriteFileEx().



#ifndef     HARDWARELIB_DEVICE_H
#define     HARDWARELIB_DEVICE_H



//  includes

#include    "Node.h"



class	Device : public Node,
                    public Lockable
{
public:
    enum class DeviceModel
    {
        FILE_SYSTEM_DISK,           //  Disk drive which uses native host filesystem for storage
        FILE_SYSTEM_PRINTER,
        FILE_SYSTEM_PUNCH,
        FILE_SYSTEM_READER,
        FILE_SYSTEM_TAPE,           //  Tape drive which uses native host filesystem for volume persistence
    };


    enum class DeviceType
    {
        //  Devices
        DISK,                   //  Disk Device or Controller
        SYMBIONT,               //  Symbiont Device or Controller
        TAPE,                   //  Tape Device or Controller
    };


    //  identifies a particular IO operation
    enum class IoFunction
    {                           // Printer .Punch.. .Reader. ..Disk.. ..Tape.. Terminal Console.
        NONE = 0,               //
        CLOSE,                  //            X
        DELETE_LINE,            //                                                         X
        GET_INFO,               //    X       X        X        X        X        X        X
        MOVE_BLOCK,             //                                       X
        MOVE_BLOCK_BACKWARD,    //                                       X
        MOVE_FILE,              //                                       X
        MOVE_FILE_BACKWARD,     //                                       X
        READ,                   //                     X        X        X        X        X
        READ_BACKWARD,          //                                       X
        RESET,                  //                                                         X
        REWIND,                 //                                       X
        REWIND_INTERLOCK,       //                                       X
        SET_MODE,               //                                       X
        UNLOAD,                 //                                       X
        WRITE,                  //    X       X                 X        X        X        X
        WRITE_END_OF_FILE,      //                                       X
    };


	//  identifies a particular IO status
    enum class IoStatus
    {                           // Printer. .Punch.. .Reader. ..Disk.. ..Tape.. Terminal Console.
        SUCCESSFUL = 0,         //    X        X        X        X        X        X        X
        BUFFER_TOO_SMALL,       //                               X        X
        DEVICE_BUSY,            //                               X        ?
        END_OF_TAPE,            //                                        X
        FILE_MARK,              //                                        X
        INVALID_BLOCK_COUNT,    //                               X
        INVALID_BLOCK_ID,       //                               X
        INVALID_BLOCK_SIZE,     //                               X
        INVALID_DEVICE_ADDRESS, //    X        X        X        X        X        X        X       // returned by controller
        INVALID_FUNCTION,       //    X        X        X        X        X
        INVALID_LINE_NUMBER,    //                                                          X
        INVALID_MODE,           //                                        X
        INVALID_TRANSFER_SIZE,  //                                        X
        IN_PROGRESS,            //                               X
        LOST_POSITION,          //                                        X
        MEDIA_ERROR,            //                      X
        NO_DEVICE,              //    X        X        X        X        X        X        X       // returned by controller
        NO_INPUT,               //                      X                          X        X
        NOT_PREPPED,            //                               X
        NOT_READY,              //    X        X        X        X        X
        QUEUE_FULL,             //                               X
        SYSTEM_EXCEPTION,       //    X        X        X        X        X
        UNIT_ATTENTION,         //                               X        ?
        WRITE_PROTECTED,        //                               X        X
    };


    //  This struct (or a superset thereof) is returned by IOF_GET_INFO
    struct DeviceInfo
    {
        DeviceModel             m_DeviceModel;
        DeviceType              m_DeviceType;
    };


	//  Describes an IO at the device level.
    class IoInfo
	{
    private:
        Node* const             m_pSource;          //  Source of the IO request
        const BLOCK_ID          m_BlockId;          //  For device-addressed functions
        const COUNT             m_ByteCount;        //  Number of bytes to be transferred
        COUNT                   m_BytesTransferred; //  Number of bytes successfully transferred
        BYTE* const             m_pBuffer;          //  Pointer to byte buffer for data transfers
        const IoFunction        m_Function;         //  Function to be performed
        IoStatus                m_Status;           //  Result of operation
        SYSTEMERRORCODE         m_SystemError;      //  System error code, for system exceptions
        VBYTE                   m_SenseBytes;       //  For physical IOs (if we ever do this)

    public:
        IoInfo( Node* const         pSource,
                const IoFunction    ioFunction,
                BYTE* const         pBuffer,
                const BLOCK_ID      blockId,
                const COUNT         byteCount )
            :m_pSource( pSource ),
            m_BlockId( blockId ),
            m_ByteCount( byteCount ),
            m_BytesTransferred( 0 ),
            m_pBuffer( pBuffer ),
            m_Function( ioFunction ),
            m_Status( IoStatus::SUCCESSFUL ),
            m_SystemError( 0 )
        {}

        inline BLOCK_ID                 getBlockId() const              { return m_BlockId; }
        inline BYTE*                    getBuffer() const               { return m_pBuffer; }
        inline COUNT                    getByteCount() const            { return m_ByteCount; }
        inline COUNT                    getBytesTransferred() const     { return m_BytesTransferred; }
        inline IoFunction               getFunction() const             { return m_Function; }
        inline const VBYTE&             getSenseBytes() const           { return m_SenseBytes; }
        inline Node* const              getSource() const               { return m_pSource; }
        inline IoStatus                 getStatus() const               { return m_Status; }
        inline SYSTEMERRORCODE          getSystemError() const          { return m_SystemError; }

        inline void setBytesTransferred( const COUNT bytes )            { m_BytesTransferred = bytes; }
        inline void setStatus( const IoStatus ioStatus )                { m_Status = ioStatus; }
        inline void setSystemError( const SYSTEMERRORCODE systemError ) { m_SystemError = systemError; }
	};


protected:
    DeviceModel                         m_DeviceModel;
    DeviceType                          m_DeviceType;

    //  Ready flag indicates the device can do reads and writes.
    //  If not ready, the device *may* respond to non-read/write IOs.
    bool                                m_ReadyFlag;

    //  Unit attention flag indicates that something about the physical characteristics
    //  of the device has changes.  Setting a device ready should ALWAYS set this flag.
    //  All reads and writes will be rejected with IOS_UNIT_ATTENTION until an IOF_GET_INFO is issued.
    bool                                m_UnitAttentionFlag;

    //  normal functions
    void                                ioEnd( const IoInfo* const ) const;
    void                                ioStart( const IoInfo* const );

    //  abstract functions
    virtual void                        writeBuffersToLog( const IoInfo* const ) const = 0;

    Device( const DeviceType    deviceType,
            const DeviceModel   deviceModel,
            const std::string&  name )
    :Node( Node::Category::DEVICE, name ),
            m_DeviceModel( deviceModel ),
            m_DeviceType( deviceType ),
            m_ReadyFlag( false ),
            m_UnitAttentionFlag( false )
    {}


public:
    virtual ~Device(){}

    virtual void                        handleIo( IoInfo* const ) = 0;
    virtual bool                        setReady( const bool );

    //  inlines
    inline DeviceModel                  getDeviceModel() const  { return m_DeviceModel; }
    inline std::string                  getDeviceModelString() const
    {
        return getDeviceModelString( m_DeviceModel );
    }
    inline DeviceType                   getDeviceType() const   { return m_DeviceType; }
    inline std::string                  getDeviceTypeString() const
    {
        return getDeviceTypeString( m_DeviceType );
    }

    //  Node interface
    virtual void                        dump( std::ostream& stream ) const = 0;
    virtual void                        initialize(){}
    virtual void                        terminate(){}

	//  inlines
    inline bool                         isReady() const         { return m_ReadyFlag; }

	//  statics
    static const char*                  getDeviceModelString( const DeviceModel );
    static const char*                  getDeviceTypeString( const DeviceType );
    static const char*                  getIoFunctionString( const IoFunction );
    static std::string                  getIoInfoString( const IoInfo* const pIoInfo );
    static std::string                  getIoStatusString( const IoStatus, const SYSTEMERRORCODE );

    //  inline statics
    static inline bool                  isReadFunction( const IoFunction ioFunction )
    {
        return (ioFunction == IoFunction::READ || ioFunction == IoFunction::READ_BACKWARD);
    };

    static inline bool                  isWriteFunction( const IoFunction ioFunction )
    {
        return (ioFunction == IoFunction::WRITE || ioFunction == IoFunction::WRITE_END_OF_FILE);
    }
};



#endif

