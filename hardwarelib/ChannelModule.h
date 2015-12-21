//  ChannelModule.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  This is where all the conversion is done from a low-level Exec IO request,
//  to device IO requests (including data conversions/packing).
//
//  Currently, all ChannelModule's have a Byte interface on the back-end.
//  If we decide to implement a Word Channel Module (why would we?)...
//  we'll have to figure out how to abstract some of this stuff into a base class,
//  with the functional stuff in superclasses.



#ifndef     HARDWARELIB_CHANNEL_MODULE_H
#define     HARDWARELIB_CHANNEL_MODULE_H



#include    "Device.h"
#include    "IoAccessControlList.h"



class   ChannelModule : public Node,
                        public Worker,
                        public Lockable
{
public:
    //  In a real world, most of these discrete commands are actually folded into very basic
    //  transfer-in, transfer-out, and util commands, with appropriate sub-commands.
    //  Its not worth the trouble to emulate at that level.
    enum class Command
    {
        NOP,          //  Special case for object constructor only
        INQUIRY,
        READ,
        WRITE,
        //TODO:TAPE more for tapes, later
    };


    //  Indicates how 36-bit data is translated (by the channel module) into byte streams
    enum class IoTranslateFormat
    {
        A,          //  For 9-track tapes, mainly.
                    //      On output, the 8 LSBits of each quarter-word are encoded per output byte.
                    //          Transfer is terminated whenever MSBit of any quarter-word is set.
                    //      On input, subsequent bytes are read into 8 LSBits of succeding words,
                    //          with MSBit set to 0.
        B,          //  For 7-track tapes.
                    //      On output, each sixth-word is encoded into a byte, with bits 7 and 8 cleared.
                    //      On input, bits 7 and 8 from the channel are ignored.
        C,          //  Most IO is done via this.
                    //      Each 36-bit word is packed into 4.5 bytes of output.  Generally, transfers are
                    //          expected to be done in increments of 2 words.
                    //      On input, subsequent bytes are read into 36-bit words, 9 bytes->2 words.
        D,          //  Same as A Format, except that on output the MSB of each quarter-word is ignored.
    };


    //  Status of a channel command
    enum class Status
    {
        SUCCESSFUL,
        CANCELLED,
        DEVICE_ERROR,
        INVALID_CHANNEL_MODULE_ADDRESS,
        INVALID_CONTROLLER_ADDRESS,
        INVALID_UPI_NUMBER,
        INSUFFICIENT_BUFFERS,
        IN_PROGRESS,
    };


    struct ChannelProgram
    {
    public:
        Worker*                 m_pSource;

        PROCESSOR_UPI           m_ProcessorUPI;
        NODE_ADDRESS            m_ChannelModuleAddress;
        NODE_ADDRESS            m_ControllerAddress;
        NODE_ADDRESS            m_DeviceAddress;

        Command                 m_Command;
        COUNT64                 m_Address;              //  could mean anything, really, depending on target
                                                        //      for DiskDevice, this is block-id
        WORD_COUNT              m_TransferSizeWords;    //  Must be <= aggregate size of all ACWs (in words) for IO
        IoAccessControlList     m_AccessControlList;
        IoTranslateFormat       m_Format;

        //  Values returned by io handler
        COUNT                   m_BytesTransferred;
        COUNT                   m_WordsTransferred;
        Status                  m_ChannelStatus;
        Device::IoStatus        m_DeviceStatus;
        SYSTEMERRORCODE         m_SystemErrorCode;
        VBYTE                   m_SenseBytes;

        ChannelProgram( Worker* const pSource )
            :m_pSource( pSource ),
            m_ProcessorUPI( 0 ),
            m_ChannelModuleAddress( 0 ),
            m_ControllerAddress( 0 ),
            m_DeviceAddress( 0 ),
            m_Command( Command::NOP ),
            m_Address( 0 ),
            m_TransferSizeWords( 0 ),
            m_Format( IoTranslateFormat::A ),
            m_BytesTransferred( 0 ),
            m_WordsTransferred( 0 ),
            m_ChannelStatus( Status::SUCCESSFUL ),
            m_DeviceStatus( Device::IoStatus::SUCCESSFUL ),
            m_SystemErrorCode( 0 )
        {}
    };

    class  ConversionBuffer
    {
    public:
        BYTE*                   m_pBuffer;
        COUNT                   m_BufferSizeBytes;
        bool                    m_InUse;

        ConversionBuffer( const COUNT requestedSizeBytes )
            :m_pBuffer( new BYTE[requestedSizeBytes] ),
            m_BufferSizeBytes( requestedSizeBytes ),
            m_InUse( false )
        {}

        ~ConversionBuffer()
        {
            delete[] m_pBuffer;
        }
    };

    typedef     std::list<ConversionBuffer*>                CONVERSIONBUFFERS;
    typedef     CONVERSIONBUFFERS::iterator                 ITCONVERSIONBUFFERS;
    typedef     CONVERSIONBUFFERS::const_iterator           CITCONVERSIONBUFFERS;

    class   Tracker
    {
    public:
        bool                    m_Cancelled;        //  caller cancelled the IO; m_pChannelProgram will be null
        ChannelProgram*         m_pChannelProgram;
        ConversionBuffer*       m_pConversionBuffer;
        COUNT                   m_TransferSizeBytes;
        Device::IoInfo*         m_pChildIo;

        Tracker( ChannelProgram* const pChannelProgram )
            :m_Cancelled( false ),
            m_pChannelProgram( pChannelProgram ),
            m_pConversionBuffer( 0 ),
            m_TransferSizeBytes( 0 ),
            m_pChildIo( 0 )
        {}
    };

    typedef     std::list<Tracker>                          TRACKERS;
    typedef     TRACKERS::iterator                          ITTRACKERS;
    typedef     TRACKERS::const_iterator                    CITTRACKERS;


private:
    CONVERSIONBUFFERS           m_ConversionBuffers;
    bool                        m_SkipDataFlag;         // From Configurator, via DeviceManager (at startup)
    TRACKERS                    m_Trackers;

    bool                assignBuffer( ITTRACKERS itTracker );
    bool                assignBuffers();
    bool                checkChildIOs();
    bool                startChildIOs();
    void                translateFromA( Tracker* const  pTracker,
                                        COUNT* const    pResidue );
    void                translateFromB( Tracker* const  pTracker,
                                        COUNT* const    pResidue );
    void                translateFromC( Tracker* const  pTracker,
                                        COUNT* const    pResidue );
    void                translateToA( Tracker* const    pTracker,
                                      const bool        ignoreMSBits );
    void                translateToB( Tracker* const pTracker );
    void                translateToC( Tracker* const pTracker );

    //  Worker interface
    void                worker();


public:
    ChannelModule( const std::string& name )
        :Node( Node::Category::CHANNEL_MODULE, name ),
        Worker( name ),
        m_SkipDataFlag( false )
    {}

    ~ChannelModule();

    bool                cancelIo( const ChannelProgram* const pChannelProgram );
    void                handleIo( ChannelProgram* const pChannelProgram );

    //  Node interface
    void                dump( std::ostream& stream ) const;
    void                initialize();
    void                signal( Node* const pSource );
    void                terminate();

    //  inlines
    inline void         setSkipDataFlag( const bool flag )      { m_SkipDataFlag = flag; }

    inline bool startUp()
    {
        return workerStart();
    }

    inline static bool isTransferCommand( const Command command )
    {
        return isTransferInCommand( command ) || isTransferOutCommand( command );
    }

    inline static bool isTransferInCommand( const Command command )
    {
        return (command == Command::INQUIRY) || (command == Command::READ);
    }

    inline static bool isTransferOutCommand( const Command command )
    {
        return (command == Command::WRITE);
    }

    //  statics
    static COUNT        getByteCountFromWordCount( const WORD_COUNT         wordCount,
                                                   const IoTranslateFormat  translateFormat );
    static const char*  getCommandString( const Command command );
    static const char*  getStatusString( const Status status );
    static const char*  getIoTranslateFormat( const IoTranslateFormat format );
};



#endif

