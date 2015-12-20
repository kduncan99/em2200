//  SymbiontBuffer.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Represents a Symbiont buffer in the RunInfo object
//
//  Note: The name of the READ$ file is always SYS$*READ$Xrun-id
//  Config PRSPMX is maximum line spacing permitted for each print request (max is 03777)
//  TODO Somewhere we need to track head, after-heading space, lines-per-page, all that crap.



#ifndef     EXECLIB_SYMBIONT_BUFFER_H
#define     EXECLIB_SYMBIONT_BUFFER_H



class SymbiontBuffer
{
public:
    enum BufferType
    {
        BUFTYP_READ,
        BUFTYP_PRINT,
        BUFTYP_PUNCH,
        BUFTYP_ADD,
        BUFTYP_RSI,
    };

    enum CharacterSet
    {
        CSET_FIELDATA = 0,
        CSET_ASCII = 1,
    };

    enum SDFType
    {
        SDFT_UNSPECIFIED =  000,        //  Fieldata '@'
        SDFT_CARD =         010,        //  Fieldata 'C', READ$ or PUNCH$
        SDFT_FILE =         016,        //  Fieldata 'I', @FILE
        SDFT_FTP =          001,        //  Fieldata '[', FTP
        SDFT_PRINT =        025,        //  Fieldata 'P', PRINT$
        SDFT_GENERAL =      030,        //  Fieldata 'S', General Symbolic
        SDFT_PCIOS =        035,        //  Fieldata 'X', PCIOS
    };

private:
    Word36* const           m_pBuffer;
    const COUNT             m_BufferSize;           //  size of buffer in words
    const BufferType        m_BufferType;
    bool                    m_CharacterSetSpecified;
    CharacterSet            m_CurrentCharacterSet;
    bool                    m_EndOfFileFlag;
    INDEX                   m_NextRead;             //  next word to be read from the buffer
    INDEX                   m_NextWrite;            //  next word to be written into the buffer
    SDFType                 m_SDFType;

protected:
    SymbiontBuffer( const BufferType    type,
                    const COUNT         size );

public:
    virtual ~SymbiontBuffer();

    void                    debug( const std::string& caption ) const;
    const Word36* const     readWord();
    bool                    readWords( Word36* const    pDestination,
                                        const COUNT     wordCount );
    bool                    writeWord( const Word36& value );
    bool                    writeWords( const Word36* const pSource,
                                        const COUNT         wordCount );

    //  public in case the calling entity needs to know this
    inline void             advanceReadIndex( const COUNT delta )   { m_NextRead += delta; }    // be careful with this!
    inline void             advanceWriteIndex( const COUNT delta )  { m_NextWrite += delta; }   // be careful with this!
    inline Word36*          getBuffer() const                       { return m_pBuffer; }       // be careful with this!
    inline COUNT            getBufferSize() const                   { return m_BufferSize; }
    inline BufferType       getBufferType() const                   { return m_BufferType; }
    inline CharacterSet     getCurrentCharacterSet() const          { return m_CurrentCharacterSet; }
    inline INDEX            getNextRead() const                     { return m_NextRead; }
    inline INDEX            getNextWrite() const                    { return m_NextWrite; }
    inline COUNT            getRemainingRead() const                { return m_NextWrite - m_NextRead; }
    inline COUNT            getRemainingWrite() const               { return m_BufferSize - m_NextWrite; }
    inline SDFType          getSDFType() const                      { return m_SDFType; }
    inline bool             isCharacterSetSpecified() const         { return m_CharacterSetSpecified; }
    inline bool             isEndOfFile() const                     { return m_EndOfFileFlag; }
    inline bool             isExhausted() const                     { return m_NextRead == m_NextWrite; }

    inline void clear()
    {
        m_NextRead = 0;
        m_NextWrite = 0;
    }

    inline void setCurrentCharacterSet( const CharacterSet cset )
    {
        m_CurrentCharacterSet = cset;
        m_CharacterSetSpecified = true;
    }

    inline void setEndOfFile()
    {
        m_EndOfFileFlag = true;
    }

    inline void setSDFType( const SDFType sdfType )
    {
        m_SDFType = sdfType;
    }

    static std::string          getBufferTypeString( const BufferType bufferType );
    static std::string          getCharacterSetString( const CharacterSet characterSet );
    static std::string          getSDFTypeString( const SDFType sdfType );
};


class   FileSymbiontBuffer : public SymbiontBuffer
{
protected:
    FacilityItem* const     m_pFacilityItem;        //  pointer to assigned FacItem for I/O; 0 for RSI I/O
    COUNT                   m_NextSectorAddress;    //  Address for next read or write I/O
    bool                    m_ReleaseFlag;          //  File is candidate for @FREE when buffer is released

public:
    FileSymbiontBuffer( const BufferType    bufferType,
                        FacilityItem* const pFacilityItem,
                        const bool          releaseFlag )
        :SymbiontBuffer( bufferType, 224 ),
        m_pFacilityItem( pFacilityItem ),
        m_ReleaseFlag( releaseFlag )
    {}

    ~FileSymbiontBuffer(){};

    inline FacilityItem*    getFacilityItem() const     { return m_pFacilityItem; }
    inline bool             getReleaseFlag() const      { return m_ReleaseFlag; }
};


class   RSISymbiontBuffer : public SymbiontBuffer
{
private:
    const COUNT             m_RSISessionNumber;

public:
    RSISymbiontBuffer( const COUNT rsiSessionNumber )
        :SymbiontBuffer( BUFTYP_RSI, 1792 ),
        m_RSISessionNumber( rsiSessionNumber )
    {}

    ~RSISymbiontBuffer(){};

    inline COUNT            getRSISessionNumber() const { return m_RSISessionNumber; }
};



#endif
