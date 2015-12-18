//  FileSystemDiskDevice.h
//
//  Class for emulating a disk device in a system file
//
//  Disk device which uses host system files as media.
// 
//  The host system file name is not of concern to us, except that we need it
//  in order to 'mount' the media - i.e., we do an OpenFile type of command,
//  and then issue reads/writes as appropriate.
// 
//  The mount command is not in the IO chain - it is conceptually an out-of-band action.



#ifndef     HARDWARELIB_FILESYSTEM_DISK_DEVICE_H
#define     HARDWARELIB_FILESYSTEM_DISK_DEVICE_H



//  includes

#include    "DiskDevice.h"



//  constants



//  macros



//  enums



//  typedefs



//  class declarations

class	FileSystemDiskDevice : public DiskDevice
{
private:
    struct  ScratchPad
    {
    public:
        BYTE                    m_Identifier[8];
        BYTE                    m_MajorVersion[4];
        BYTE                    m_MinorVersion[4];
        BYTE                    m_BlockSize[sizeof(BLOCK_SIZE)];
        BYTE                    m_BlockCount[sizeof(BLOCK_COUNT)];
    };

    class   ScratchPadInfo
    {
    public:
        std::string             m_Identifier;
        UINT32                  m_MajorVersion;
        UINT32                  m_MinorVersion;
        PREP_FACTOR             m_PrepFactor;
        BLOCK_SIZE              m_BlockSize;
        BLOCK_COUNT             m_BlockCount;
    };

    SimpleFile*                 m_pSimpleFile;

    //  private methods
    COUNT64                     calculateByteOffset( const BLOCK_ID blockId ) const;
    bool                        readScratchPadInfo( ScratchPadInfo* const pInfo ) const;
    bool                        writeScratchPadInfo( const ScratchPadInfo& info ) const;


	//  DiskDevice interface
	void						ioRead( IoInfo* const );
	void						ioReset( IoInfo* const );
	void						ioUnload( IoInfo* const );
	void						ioWrite( IoInfo* const );


    //  static inline private methods                                
    static inline BLOCK_COUNT deserializeBlockCount( const BYTE value[8] )
    {
        return deserializeUint64( value );
    }

    static inline BLOCK_SIZE deserializeBlockSize( const BYTE value[4] )
    {
        return deserializeUint32( value );
    }

    static inline void deserializeScratchPad( ScratchPadInfo* const     pInfo,
                                                const ScratchPad* const pPad )
    {
        char tempString[sizeof(pPad->m_Identifier) + 1];
        memcpy( tempString, pPad->m_Identifier, sizeof(pPad->m_Identifier) );
        tempString[sizeof(pPad->m_Identifier)] = 0x00;

        pInfo->m_Identifier = tempString;
        pInfo->m_MajorVersion = deserializeUint32( pPad->m_MajorVersion );
        pInfo->m_MinorVersion = deserializeUint32( pPad->m_MinorVersion );
        pInfo->m_BlockSize = deserializeBlockSize( pPad->m_BlockSize );
        pInfo->m_BlockCount = deserializeBlockCount( pPad->m_BlockCount );
    }

    static inline UINT32 deserializeUint32( const BYTE value[4] )
    {
        return (static_cast<UINT32>(value[0]) << 24)
            | (static_cast<UINT32>(value[1]) << 16)
            | (static_cast<UINT32>(value[2]) << 8)
            | (static_cast<UINT32>(value[3]));
    }

    static inline UINT64 deserializeUint64( const BYTE value[8] )
    {
        return (static_cast<UINT64>(value[0]) << 56)
            | (static_cast<UINT64>(value[1]) << 48)
            | (static_cast<UINT64>(value[2]) << 40)
            | (static_cast<UINT64>(value[3]) << 32)
            | (static_cast<UINT64>(value[4]) << 24)
            | (static_cast<UINT64>(value[5]) << 16)
            | (static_cast<UINT64>(value[6]) << 8)
            | (static_cast<UINT64>(value[7]));
    }

    static inline void serializeScratchPad( ScratchPad* const           pPad,
                                            const ScratchPadInfo* const pInfo )
    {
        memset( pPad, 0, sizeof(ScratchPad) );
        for ( INDEX cx = 0; cx < pInfo->m_Identifier.size() && cx < sizeof(pPad->m_Identifier); ++cx )
            pPad->m_Identifier[cx] = pInfo->m_Identifier[cx];
        serializeUint32( pPad->m_MajorVersion, pInfo->m_MajorVersion );
        serializeUint32( pPad->m_MinorVersion, pInfo->m_MinorVersion );
        serializeBlockSize( pPad->m_BlockSize, pInfo->m_BlockSize );
        serializeBlockCount( pPad->m_BlockCount, pInfo->m_BlockCount );
    }

    static inline void serializeBlockCount( BYTE* const pResult, const BLOCK_COUNT value )
    {
        serializeUint64( pResult, value );
    }

    static inline void serializeBlockSize( BYTE* const pResult, const BLOCK_SIZE value )
    {
        serializeUint32( pResult, value );
    }

    static inline void serializeUint32( BYTE* const pResult, const UINT32 value )
    {
        *pResult = static_cast<BYTE>(value >> 24);
        *(pResult + 1) = static_cast<BYTE>(value >> 16);
        *(pResult + 2) = static_cast<BYTE>(value >> 8);
        *(pResult + 3) = static_cast<BYTE>(value & 0xff);
    }

    static inline void serializeUint64( BYTE* const pResult, const UINT64 value )
    {
        *pResult = static_cast<BYTE>(value >> 56);
        *(pResult + 1) = static_cast<BYTE>(value >> 48);
        *(pResult + 2) = static_cast<BYTE>(value >> 40);
        *(pResult + 3) = static_cast<BYTE>(value >> 32);
        *(pResult + 4) = static_cast<BYTE>(value >> 24);
        *(pResult + 5) = static_cast<BYTE>(value >> 16);
        *(pResult + 6) = static_cast<BYTE>(value >> 8);
        *(pResult + 7) = static_cast<BYTE>(value & 0xff);
    }

public:
    FileSystemDiskDevice( const std::string& name )
        :DiskDevice( Device::DeviceModel::FILE_SYSTEM_DISK, name ),
        m_pSimpleFile( 0 )
        {}

	virtual ~FileSystemDiskDevice();

	bool						mount( const std::string& mediaName );
    bool                        setReady( const bool flag );
	bool						unmount();

    inline std::string          getPackName() const
    {
        if ( isMounted() )
            return m_pSimpleFile->getFileName();
        else
            return "";
    }

    static bool                 createPack( const std::string&  fileName,
                                            const BLOCK_SIZE    blockSize,
                                            const BLOCK_COUNT   blockCount,
                                            std::string* const  pErrorMessage );

    //  Node interface
    void                        dump( std::ostream& ) const;
};



#endif

