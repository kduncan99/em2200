//  SymbiontBuffer.cpp


#include    "execlib.h"



//  private, protected methods



//  constructors, destructors

SymbiontBuffer::SymbiontBuffer
(
    const BufferType    type,
    const COUNT         size
)
:m_pBuffer( new Word36[size] ),
m_BufferSize( size ),
m_BufferType( type )
{
    clear();
    m_CharacterSetSpecified = false;
    m_CurrentCharacterSet = CSET_FIELDATA;
    m_EndOfFileFlag = false;
    m_SDFType = SDFT_UNSPECIFIED;
}


SymbiontBuffer::~SymbiontBuffer()
{
    delete[] m_pBuffer;
}



//  public methods

//  debug()
void
SymbiontBuffer::debug
(
    const std::string&  caption
) const
{
    //TODO:DEBUG
    std::cout << caption << " -- "
        << "NextRead:" << m_NextRead
        << " NextWrite:" << m_NextWrite << std::endl;

    for ( INDEX x = m_NextRead; x < m_NextWrite; ++x )
    {
        std::cout << x << " : "
            << m_pBuffer[x].toOctal() << "  "
            << m_pBuffer[x].toFieldata() << "  "
            << m_pBuffer[x].toAscii() << std::endl;
    }
}


//  readWord()
//
//  Reads the next un-read word from the buffer.
//  If the buffer is exhausted, we return 0.
//  If we read the last available word, we reset the indices.
//
//  This is a special version which actually just returns a pointer into the buffer.
//  That means the caller *must* be carefully using the pointer - it won't be valid
//  after any other read or write.
const Word36* const
SymbiontBuffer::readWord()
{
    const Word36* pWord = 0;
    if ( !isExhausted() && !isEndOfFile() )
    {
        pWord = m_pBuffer + m_NextRead;
        ++m_NextRead;
        if ( isExhausted() )
            clear();
    }
    return pWord;
}


//  readWords()
//
//  Transfers the requested number of words from the buffer
//  to the caller's destination, updating m_NextRead appropriately.
//
//  Returns false if the caller requests more words than are left to be read.
bool
SymbiontBuffer::readWords
(
    Word36* const   pDestination,
    const COUNT     wordCount
)
{
    if ( isEndOfFile() || ( getRemainingRead() < wordCount ) )
        return false;

    for ( INDEX wx = 0; wx < wordCount; ++wx, ++m_NextRead )
        pDestination[wx] = m_pBuffer[m_NextRead];
    return true;
}


//  writeWord()
//
//  Writes the provided word into the buffer.
//  If the buffer is full, we return false.
bool
SymbiontBuffer::writeWord
(
    const Word36&       value
)
{
    bool result = false;

    if ( m_NextWrite < m_BufferSize )
    {
        m_pBuffer[m_NextWrite] = value;
        ++m_NextWrite;
        result = true;
    }

    return result;
}


//  writeWords()
//
//  Writes the words from the caller's source into the buffer,
//  updating m_NextWrite appropriately.
//  If there is insufficient buffer space, we return false
//  (having transferred nothing).
bool
SymbiontBuffer::writeWords
(
    const Word36* const pSource,
    const COUNT         wordCount
)
{
    if ( getRemainingWrite() < wordCount )
        return false;

    for ( INDEX wx = 0; wx < wordCount; ++wx, ++m_NextWrite )
        m_pBuffer[m_NextWrite] = pSource[wx];
    return true;
}



//  static public methods

//  getBufferTypeString()
std::string
SymbiontBuffer::getBufferTypeString
(
    const BufferType        bufferType
)
{
    switch ( bufferType )
    {
    case BUFTYP_READ:       return "READ";
    case BUFTYP_PRINT:      return "PRINT";
    case BUFTYP_PUNCH:      return "PUNCH";
    case BUFTYP_ADD:        return "ADD";
    case BUFTYP_RSI:        return "RSI";
    };

    return "???";
}


//  getCharacterSetString()
std::string
SymbiontBuffer::getCharacterSetString
(
    const CharacterSet      characterSet
)
{
    switch ( characterSet )
    {
    case CSET_FIELDATA:     return "FIELDATA";
    case CSET_ASCII:        return "ASCII";
    };

    return "???";
}


//  getSDFTypeString()
std::string
SymbiontBuffer::getSDFTypeString
(
    const SDFType           sdfType
)
{
    switch ( sdfType )
    {
    case SDFT_UNSPECIFIED:  return "UNSPECIFIED(@)";
    case SDFT_CARD:         return "CARD(C)";
    case SDFT_FILE:         return "@FILE(I)";
    case SDFT_FTP:          return "FTP([)";
    case SDFT_PRINT:        return "PRINT(P)";
    case SDFT_GENERAL:      return "GENERAL(S)";
    case SDFT_PCIOS:        return "PCIOS(X)";
    };

    return "???";
}

