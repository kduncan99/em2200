//  generally useful static methods



#include    "misclib.h"



//  static tables
static char AsciiFromFieldata[] =
{
	'@',	'[',	']',	'#',	'^',	' ',	'A',	'B',
	'C',	'D',	'E',	'F',	'G',	'H',	'I',	'J',
	'K',	'L',	'M',	'N',	'O',	'P',	'Q',	'R',
	'S',	'T',	'U',	'V',	'W',	'X',	'Y',	'Z',
	')',	'-',	'+',	'<',	'=',	'>',	'&',	'$',
	'*',	'(',	'%',	':',	'?',	'!',	',',	'\\',
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	'\'',	';',	'/',	'.',	'"',	'_',
};


static BYTE FieldataFromAscii[] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x05, 0x2D, 0x3E, 0x03, 0x27, 0x2A, 0x26, 0x3A, 0x29, 0x20, 0x28, 0x22, 0x2E, 0x21, 0x3D, 0x3C,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x3B, 0x23, 0x24, 0x25, 0x2c,
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x01, 0x2F, 0x02, 0x04, 0x3F,
    0xFF, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};



//  external functions

//  miscAdd12()
//
//  Performs 12-bit ones-complement arithmetic addition
//  Parameters:
//      operand1:       ones-complement signed addend
//      operand2:       ones-complement signed addend
//      pCarry:         where we store the carry flag (can be null)
//      pOverflow:      where we store the overflow flag (can be null)
//  Returns:
//      Ones-complement signed sum
UINT16
    miscAdd12
    (
    const UINT16    operand1,
    const UINT16    operand2,
    bool* const     pCarry,
    bool* const     pOverflow
    )
{
    if ( (operand1 == 07777) && (operand2 == 07777)  )
        return 07777;

    INT16 nativeOp1 = miscTwosComplement12( operand1 );
    INT16 nativeOp2 = miscTwosComplement12( operand2 );
    INT16 nativeResult = nativeOp1 + nativeOp2;

    if ( pCarry || pOverflow )
    {
        bool op1Neg = nativeOp1 < 0;
        bool op2Neg = nativeOp2 < 0;
        bool resultNeg = nativeResult < 0;

        // The carry bit is always set if both operands are negative, clear if both operands are positive,
        // and set if the result is positive while the signs of the operands do not match.
        if ( pCarry )
            *pCarry = (op1Neg && op2Neg) || ((op1Neg != op2Neg) && !resultNeg );

        // The overflow bit is always set if the operands' signs match each other,
        // but do not match the sign of the result.
        if ( pOverflow )
            *pOverflow = (op1Neg == op2Neg) && (op1Neg != resultNeg);
    }

    return miscOnesComplement12( nativeResult );
}


//  miscAdd18()
//
//  Performs 18-bit ones-complement arithmetic addition
//  Parameters:
//      operand1:       ones-complement signed addend
//      operand2:       ones-complement signed addend
//      pCarry:         where we store the carry flag (can be null)
//      pOverflow:      where we store the overflow flag (can be null)
//  Returns:
//      Ones-complement signed sum
UINT32
    miscAdd18
    (
    const UINT32    operand1,
    const UINT32    operand2,
    bool* const     pCarry,
    bool* const     pOverflow
    )
{
    if ( (operand1 == 0777777) && (operand2 == 0777777)  )
        return 0777777;

    INT32 nativeOp1 = miscTwosComplement18( operand1 );
    INT32 nativeOp2 = miscTwosComplement18( operand2 );
    INT32 nativeResult = nativeOp1 + nativeOp2;

    if ( pCarry || pOverflow )
    {
        bool op1Neg = nativeOp1 < 0;
        bool op2Neg = nativeOp2 < 0;
        bool resultNeg = nativeResult < 0;

        // The carry bit is always set if both operands are negative, clear if both operands are positive,
        // and set if the result is positive while the signs of the operands do not match.
        if ( pCarry )
            *pCarry = (op1Neg && op2Neg) || ((op1Neg != op2Neg) && !resultNeg );

        // The overflow bit is always set if the operands' signs match each other,
        // but do not match the sign of the result.
        if ( pOverflow )
            *pOverflow = (op1Neg == op2Neg) && (op1Neg != resultNeg);
    }

    return miscOnesComplement18( nativeResult );
}


//  miscAdd24()
//
//  Performs 24-bit ones-complement arithmetic addition
//  Parameters:
//      operand1:       ones-complement signed addend
//      operand2:       ones-complement signed addend
//      pCarry:         where we store the carry flag (can be null)
//      pOverflow:      where we store the overflow flag (can be null)
//  Returns:
//      Ones-complement signed sum
UINT32
    miscAdd24
    (
    const UINT32    operand1,
    const UINT32    operand2,
    bool* const     pCarry,
    bool* const     pOverflow
    )
{
    if ( (operand1 == 077777777) && (operand2 == 077777777)  )
        return 077777777;

    INT32 nativeOp1 = miscTwosComplement24( operand1 );
    INT32 nativeOp2 = miscTwosComplement24( operand2 );
    INT32 nativeResult = nativeOp1 + nativeOp2;

    if ( pCarry || pOverflow )
    {
        bool op1Neg = nativeOp1 < 0;
        bool op2Neg = nativeOp2 < 0;
        bool resultNeg = nativeResult < 0;

        // The carry bit is always set if both operands are negative, clear if both operands are positive,
        // and set if the result is positive while the signs of the operands do not match.
        if ( pCarry )
            *pCarry = (op1Neg && op2Neg) || ((op1Neg != op2Neg) && !resultNeg );

        // The overflow bit is always set if the operands' signs match each other,
        // but do not match the sign of the result.
        if ( pOverflow )
            *pOverflow = (op1Neg == op2Neg) && (op1Neg != resultNeg);
    }

    return miscOnesComplement24( nativeResult );
}


//  miscAdd36()
//
//  Performs 36-bit ones-complement arithmetic addition
//  Parameters:
//      operand1:       ones-complement signed addend
//      operand2:       ones-complement signed addend
//      pCarry:         where we store the carry flag (can be null)
//      pOverflow:      where we store the overflow flag (can be null)
//  Returns:
//      Ones-complement signed sum
UINT64
    miscAdd36
    (
    const UINT64    operand1,
    const UINT64    operand2,
    bool* const     pCarry,
    bool* const     pOverflow
    )
{
    if ( (operand1 == 0777777777777) && (operand2 == 0777777777777)  )
        return 0777777777777;

    INT64 nativeOp1 = miscTwosComplement36( operand1 );
    INT64 nativeOp2 = miscTwosComplement36( operand2 );
    INT64 nativeResult = nativeOp1 + nativeOp2;

    if ( pCarry || pOverflow )
    {
        bool op1Neg = nativeOp1 < 0;
        bool op2Neg = nativeOp2 < 0;
        bool resultNeg = nativeResult < 0;

        // The carry bit is always set if both operands are negative, clear if both operands are positive,
        // and set if the result is positive while the signs of the operands do not match.
        if ( pCarry )
            *pCarry = (op1Neg && op2Neg) || ((op1Neg != op2Neg) && !resultNeg );

        // The overflow bit is always set if the operands' signs match each other,
        // but do not match the sign of the result.
        if ( pOverflow )
            *pOverflow = (op1Neg == op2Neg) && (op1Neg != resultNeg);
    }

    return miscOnesComplement36( nativeResult );
}


//  miscAsciiToDisplay
//
//  Filters ASCII characters into only displayable characters
//
//  Parameters:
//      ascChar:            input character
//      defaultChar:        character to be used to represent non-displayable input characters
//
//  Returns:
//      output character
char
    miscAsciiToDisplay
    (
    const char      ascChar,
    const char      defaultChar
    )
{
    return ( ascChar < 0x20 || ascChar > 0x7E ) ? defaultChar : ascChar;
}


//  miscDumpWord36Buffer
//
//  Dumps a buffer to the indicated output stream
void
miscDumpWord36Buffer
(
    std::ostream&           stream,
    const Word36* const     pBuffer,
    const COUNT             wordCount
)
{
    INDEX wx = 0;
    COUNT wordLimit = wordCount;
    while ( wordLimit % 4 )
        ++wordLimit;

    std::string octalString;
    std::string fieldataString;
    std::string asciiString;
    while ( wx < wordLimit )
    {
        if ( (wx % 4) == 0 )
            stream << std::oct << std::setw(6) << std::setfill('0') << wx << ":";

        if ( wx >= wordCount )
        {
            octalString += "             ";
            fieldataString += "       ";
            asciiString += "     ";
        }
        else
        {
            octalString += pBuffer[wx].toOctal() + " ";
            fieldataString += pBuffer[wx].toFieldata() + " ";
            asciiString += pBuffer[wx].toAscii() + " ";
        }

        if ( (wx % 4) == 3 )
        {
            stream << octalString << " " << fieldataString << " " << asciiString << std::endl;
            octalString.clear();
            fieldataString.clear();
            asciiString.clear();
        }

        ++wx;
    }
}


//  miscDumpWord36BufferToLog
//
//  Dumps a buffer to the indicated system log
void
miscDumpWord36BufferToLog
(
    SystemLog* const        pSystemLog,
    const Word36* const     pBuffer,
    const COUNT             wordCount
)
{
    INDEX wx = 0;
    COUNT wordLimit = wordCount;
    while ( wordLimit % 4 )
        ++wordLimit;

    std::stringstream strm;
    std::string octalString;
    std::string fieldataString;
    std::string asciiString;
    while ( wx < wordLimit )
    {
        if ( (wx % 4) == 0 )
        {
            strm.str( "" );
            strm << std::oct << std::setw(6) << std::setfill('0') << wx << ":";
        }

        if ( wx >= wordCount )
        {
            octalString += "             ";
            fieldataString += "       ";
            asciiString += "     ";
        }
        else
        {
            octalString += pBuffer[wx].toOctal() + " ";
            fieldataString += pBuffer[wx].toFieldata() + " ";
            asciiString += pBuffer[wx].toAscii() + " ";
        }

        if ( (wx % 4) == 3 )
        {
            strm << octalString << " " << fieldataString << " " << asciiString;
            pSystemLog->write( strm.str() );
            octalString.clear();
            fieldataString.clear();
            asciiString.clear();
        }

        ++wx;
    }
}


//  miscFieldataToAscii()
//
//  Converts a binary value representing a character in the Fieldata character set, to an equivalent ASCII character
//
//  Parameters:
//      fdChar:         fieldata character input
//
//  Returns:
//      equivalent ascii character
char
    miscFieldataToAscii
    (
    const char          fdChar
    )
{
    return AsciiFromFieldata[fdChar & 077];
}


//  miscGetAvailableMemory()
//
//  Retrieves the amount of physical RAM available in bytes
COUNT64
miscGetAvailableMemory()
{
#ifdef WIN32
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx( &memoryStatus );
    return memoryStatus.ullAvailPhys;
#else
    struct sysinfo sysInfo;
    if ( sysinfo( &sysInfo ) == -1 )
        return 0;
    return (COUNT64)(sysInfo.freeram) * sysInfo.mem_unit;
#endif
}


//  miscGetBlockSizeFromPrepFactor()
//
//  Converts prepfactor (words-per-block) to block size (bytes-per-block)
BLOCK_SIZE
miscGetBlockSizeFromPrepFactor
(
    PREP_FACTOR         prepFactor
)
{
    switch ( prepFactor )
    {
    case 28:        return 128;
    case 56:        return 256;
    case 112:       return 512;
    case 224:       return 1024;
    case 448:       return 2048;
    case 896:       return 4096;
    case 1792:      return 8192;
    }

    return 0;
}


//  miscGetErrorCode()
//
//  Retrieves the most current system error value
SYSTEMERRORCODE
miscGetErrorCode()
{
#ifdef WIN32
    return GetLastError();
#else
    return errno;
#endif
}


//  miscGetErrorCodeString()
//
//  Retrieves a string containing a canned displayable explanation
//  for the given error code, assumed to be a system error code
//
//  Parameters:
//      errorCode:          code returned from GetLastError() call
//
//  Returns:
//      string containing text
std::string
miscGetErrorCodeString
(
    const SYSTEMERRORCODE   errorCode
)
{
#ifdef WIN32
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    char* msgpointer = NULL;
    DWORD fmretn = FormatMessage( flags,
                                    NULL,
                                    errorCode,
                                    langid,
                                    reinterpret_cast<LPTSTR>(&msgpointer),
                                    0,
                                    0 );
    if (fmretn == 0)
        return "";

    char* pnewline = (char*) strstr(msgpointer, "\n");
    if (pnewline)
        *pnewline = ASCII_NUL;

    std::string retstr = msgpointer;
    LocalFree( msgpointer );

    return retstr;
#endif

#ifndef WIN32
    return strerror( errorCode );
#endif
}


//  miscGetExecIoBufferAddressModifierString()
//
//  Converts value to displayable text
const char*
miscGetExecIoBufferAddressModifierString
(
    const ExecIoBufferAddressModifier   value
)
{
    switch ( value )
    {
    case EXIOBAM_INCREMENT:     return "Increment";
    case EXIOBAM_NO_CHANGE:     return "No Change";
    case EXIOBAM_DECREMENT:     return "Decrement";
    case EXIOBAM_SKIP_DATA:     return "Skip Data";
    }

    return "???";
}


//  miscGetExecIoFunctionString()
//
//  Converts value to displayable text
const char*
miscGetExecIoFunctionString
(
    const ExecIoFunction    value
)
{
    switch ( value )
    {
    case EXIOFUNC_WRITE_BY_BDI:                 return "BDW$";
    case EXIOFUNC_BLOCK_ID_SAFE:                return "BSAFE$";
    case EXIOFUNC_READ_BLOCK_ID_BEFORE_WRITE:   return "RBDIW$";
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:        return "BDWX$";
    case EXIOFUNC_WRITE:                        return "W$";
    case EXIOFUNC_WRITE_END_OF_FILE:            return "WEF$";
    case EXIOFUNC_WRITE_ADDRESS_AND_LENGTH:     return "WAL$";
    case EXIOFUNC_GATHER_WRITE:                 return "GW$";
    case EXIOFUNC_ACQUIRE:                      return "ACQ$";
    case EXIOFUNC_EXTENDED_ACQUIRE:             return "EACQ$";
    case EXIOFUNC_READ:                         return "R$";
    case EXIOFUNC_READ_BACKWARD:                return "RB$";
    case EXIOFUNC_READ_AND_RELEASE:             return "RR$";
    case EXIOFUNC_RELEASE:                      return "REL$";
    case EXIOFUNC_BLOCK_READ_DRUM:              return "BRD$";
    case EXIOFUNC_READ_AND_LOCK:                return "RDL$";
    case EXIOFUNC_UNLOCK:                       return "UNL$";
    case EXIOFUNC_EXTENDED_RELEASE:             return "EREL$";
    case EXIOFUNC_TRACK_SEARCH_ALL:             return "TSA$";
    case EXIOFUNC_TRACK_SEARCH_FIRST:           return "TSF$";
    case EXIOFUNC_POSITION_SEARCH_ALL:          return "PSA$";
    case EXIOFUNC_POSITION_SEARCH_FIRST:        return "PSF$";
    case EXIOFUNC_SEARCH_DRUM:                  return "SD$";
    case EXIOFUNC_BLOCK_SEARCH_DRUM:            return "BSD$";
    case EXIOFUNC_SEARCH_READ_DRUM:             return "SRD$";
    case EXIOFUNC_BLOCK_SEARCH_READ_DRUM:       return "BSRD$";
    case EXIOFUNC_REWIND:                       return "REW$";
    case EXIOFUNC_REWIND_WITH_INTERLOCK:        return "REWI$";
    case EXIOFUNC_SET_MODE:                     return "SM$";
    case EXIOFUNC_SCATTER_READ:                 return "SCR$";
    case EXIOFUNC_SCATTER_READ_BACKWARD:        return "SCRB$";
    case EXIOFUNC_READ_BLOCK_IDENTIFIER:        return "RBKID$";
    case EXIOFUNC_LOCATE_BLOCK:                 return "LBLK$";
    case EXIOFUNC_READ_BY_BDI_EXTENDED:         return "BDRX$";
    case EXIOFUNC_MOVE_FORWARD:                 return "MF$";
    case EXIOFUNC_MOVE_BACKWARD:                return "MB$";
    case EXIOFUNC_FORWARD_SPACE_FILE:           return "FSF$";
    case EXIOFUNC_BACK_SPACE_FILE:              return "BSF$";
    case EXIOFUNC_READ_BY_BDI:                  return "BDR$";
    case EXIOFUNC_MODE_SET:                     return "MS$";
    case EXIOFUNC_READ_BACKWARD_BY_BDI:         return "BDRB$";
    case EXIOFUNC_FEP_INITIALIZATION:           return "FEPI$";
    case EXIOFUNC_FEP_TERMINATION:              return "FEPT$";
    case EXIOFUNC_SENSE_STATISTICS:             return "SS$";
    case EXIOFUNC_END:                          return "ENDH$";
    case EXIOFUNC_SET_TEST:                     return "ST$";
    case EXIOFUNC_MULTIREQUEST:                 return "MR$";
    case EXIOFUNC_INPUT_DATA:                   return "IND$";
    case EXIOFUNC_FILE_UPDATE_WAIT:             return "FSAFE$";
    case EXIOFUNC_LOAD_CODE_CONV_BANK:          return "LCCB$";
    }

    return "???";
}


//  miscGetPrepFactorFromBlockSize()
//
//  Converts block size (bytes-per-block) to prep factor (words-per-block)
PREP_FACTOR
miscGetPrepFactorFromBlockSize
(
    BLOCK_SIZE          blockSize
)
{
    switch ( blockSize )
    {
    case 128:       return 28;
    case 256:       return 56;
    case 512:       return 112;
    case 1024:      return 224;
    case 2048:      return 448;
    case 4096:      return 896;
    case 8192:      return 1792;
    }

    return 0;
}


//  miscGetTotalMemory()
//
//  Retrieves the total amount of physical RAM in bytes
COUNT64
miscGetTotalMemory()
{
#ifdef WIN32
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx( &memoryStatus );
    return memoryStatus.ullTotalPhys;
#else
    struct sysinfo sysInfo;
    if ( sysinfo( &sysInfo ) == -1 )
        return 0;
    return (COUNT64)(sysInfo.totalram) * sysInfo.mem_unit;
#endif
}


//  miscIsValidBlockSize()
bool
miscIsValidBlockSize
(
    const BLOCK_SIZE        blockSize
)
{
    switch ( blockSize )
    {
    case 128:
    case 256:
    case 512:
    case 1024:
    case 2048:
    case 4096:
    case 8192:
        return true;
    }

    return false;
}


//  miscIsValidNodeName()
bool
miscIsValidNodeName
(
    const std::string&      nodeName
)
{
    if ( (nodeName.size() == 0) || (nodeName.size() > 6) )
        return false;
    for ( INDEX nnx = 0; nnx < nodeName.size(); ++nnx )
    {
        if ( isalpha( nodeName[nnx] ) )
        {
            if ( islower( nodeName[nnx] ) )
                return false;
            else
            {
                // okay
            }
        }
        else if ( isdigit( nodeName[nnx] ) )
        {
            // okay
        }
        else if ( nodeName[nnx] == '-' )
        {
            //  okay
        }
        else
            return false;
    }

    return true;
}


//  miscIsValidPackName()
bool
miscIsValidPackName
(
    const std::string&      packName
)
{
    if ( (packName.size() == 0) || (packName.size() > 6) )
        return false;
    for ( INDEX pnx = 0; pnx < packName.size(); ++pnx )
    {
        if ( !isalnum( packName[pnx] ) )
            return false;
        if ( islower( packName[pnx] ) )
            return false;
    }

    return true;
}


//  miscIsValidPrepFactor()
bool
miscIsValidPrepFactor
(
    const PREP_FACTOR       prepFactor
)
{
    switch ( prepFactor )
    {
    case 28:
    case 56:
    case 112:
    case 224:
    case 448:
    case 896:
    case 1792:
        return true;
    }

    return false;
}


//  miscOnesCompAdd36()
//
//  Add two 36-bit values in ones-complement mode.
//
//  Parameters:
//      addend1:            Operand 1
//      addend2:            Operand 2
//      pCarry:             pointer to where we store the carry flag (NULL allowed)
//      pOverflow:          pointer to where we store the overflow flag (NULL allowed)
//
//  Returns:
//      sum, stored in 36-bits as ones-complement.
UINT64
miscOnesCompAdd36
(
    const UINT64&       addend1,
    const UINT64&       addend2,
    bool* const         pCarry,
    bool* const         pOverflow
)
{
    //  Clear flags
    if ( pCarry )
        *pCarry = false;
    if ( pOverflow )
        *pOverflow = false;

    //  Negative-Zero hack
    if ( ( addend1 == 0777777777777 ) && ( addend2 == 0777777777777 ) )
        return 0777777777777;

    //  Actual arithmetic
    INT64 i1 = miscTwosComplement36( addend1 );
    INT64 i2 = miscTwosComplement36( addend2 );
    INT64 sum = i1 + i2;

    // The carry bit is always set if both operands are negative, clear if both operands are positive,
    // and set if the result is positive while the signs of the operands do not match.
    if ( pCarry || pOverflow )
    {
        bool add1IsNeg = miscIsNegative36( addend1 );
        bool add2IsNeg = miscIsNegative36( addend2 );
        bool sumIsNeg = sum < 0;
        if ( pCarry )
            *pCarry = (add1IsNeg && add2IsNeg) || ( (!sumIsNeg) && (add1IsNeg != add2IsNeg) );

        // The overflow bit is always set if the operands' signs match each other,
        // but do not match the sign of the result.
        if ( pOverflow )
            *pOverflow = (add1IsNeg == add2IsNeg) && (add1IsNeg != sumIsNeg);
    }

    return miscOnesComplement36( sum );
}


//  miscOnesCompSubtract36()
//
//  Subtract one 36-bit value from another in ones-complement mode.
//
//  Parameters:
//      minuend:           Operand 1
//      subtrahend:        Operand 2
//      pCarry:             pointer to where we store the carry flag (NULL allowed)
//      pOverflow:          pointer to where we store the overflow flag (NULL allowed)
//
//  Returns:
//      sum, stored in 36-bits as ones-complement.
UINT64
miscOnesCompSubtract36
(
    const UINT64&       minuend,
    const UINT64&       subtrahend,
    bool* const         pCarry,
    bool* const         pOverflow
)
{
    return miscOnesCompAdd36( minuend, miscComplement36( subtrahend ), pCarry, pOverflow );
}


//  miscSleep()
//
//  Sleeps for a given number of milliseconds (or more, depending upon the runtime)
void
miscSleep
(
    const COUNT32 milliseconds
)
{
#ifdef WIN32
    Sleep( milliseconds );
#else
    COUNT64 micros = static_cast<COUNT64>(milliseconds) * 1000;
    usleep( micros );
#endif
}


//  miscStringToWord36Ascii()
//
//  Given a std::string containing text, we store the individual characters into successive quarter-words
//  in the given buffer.  If the string does not contain an integral number of characters (i.e., not divisible by 4)
//  we pad the last destination word with ASCII blanks.
void
miscStringToWord36Ascii
(
    const std::string&  text,
    Word36* const       pWord36Buffer
)
{
    INDEX bx = 0;   // ASCII byte index, 0 to 3
    INDEX wx = 0;   // destination word index
    INDEX sx = 0;   // source text index
    COUNT destBytes = static_cast<COUNT>(text.size());
    while ( destBytes % 4 )
        ++destBytes;

    while ( sx < destBytes )
    {
        BYTE ascChar = (sx < text.size()) ? text[sx] : ASCII_SPACE;
        switch ( bx )
        {
        case 0:
            pWord36Buffer[wx].setQ1( ascChar );
            break;
        case 1:
            pWord36Buffer[wx].setQ2( ascChar );
            break;
        case 2:
            pWord36Buffer[wx].setQ3( ascChar );
            break;
        case 3:
            pWord36Buffer[wx].setQ4( ascChar );
            break;
        }

        ++sx;
        ++bx;
        if ( bx == 4 )
        {
            bx = 0;
            ++wx;
        }
    }
}


//  miscStringToWord36Ascii()
//
//  Given a std::string containing text, we store the individual characters into successive quarter-words
//  in the given buffer.  We ensure that the amount of data transferred is equal to the
//  given number of words, padding or trimming as necessary.
void
miscStringToWord36Ascii
(
    const std::string&  text,
    Word36* const       pWord36Buffer,
    const COUNT         word36Count
)
{
    std::string tempStr = text;
    tempStr.resize( word36Count * 4, ' ' );
    miscStringToWord36Ascii( tempStr, pWord36Buffer );
}


//  miscStringToWord36Fieldata()
//
//  Given a std::string containing text, we convert the individual characters into successive fieldata bytes,
//  storing them into the given buffer.  If the string does not contain an integral number of characters
//  (i.e., not divisible by 6), we pad the last destination word with fieldata blanks.
void
    miscStringToWord36Fieldata
    (
    const std::string&  text,
    Word36* const       pWord36Buffer
    )
{
    INDEX bx = 0;   // fieldata byte index, 0 to 5
    INDEX wx = 0;   // destination word index
    INDEX sx = 0;   // source text index
    COUNT destBytes = static_cast<COUNT>(text.size());
    while ( destBytes % 6 )
        ++destBytes;

    while ( sx < destBytes )
    {
        BYTE ascChar = (sx < text.size()) ? text[sx] : ASCII_SPACE;
        BYTE fdChar = FieldataFromAscii[ascChar];
        switch ( bx )
        {
        case 0:
            pWord36Buffer[wx].setS1( fdChar );
            break;
        case 1:
            pWord36Buffer[wx].setS2( fdChar );
            break;
        case 2:
            pWord36Buffer[wx].setS3( fdChar );
            break;
        case 3:
            pWord36Buffer[wx].setS4( fdChar );
            break;
        case 4:
            pWord36Buffer[wx].setS5( fdChar );
            break;
        case 5:
            pWord36Buffer[wx].setS6( fdChar );
            break;
        }

        ++sx;
        ++bx;
        if ( bx == 6 )
        {
            bx = 0;
            ++wx;
        }
    }
}


//  miscStringToWord36Fieldata()
//
//  Given a std::string containing text, we convert the individual characters into successive fieldata bytes,
//  storing them into the given buffer.  We ensure that the amount of data transferred is equal to the
//  given number of words, padding or trimming as necessary.
void
miscStringToWord36Fieldata
(
    const std::string&  text,
    Word36* const       pWord36Buffer,
    const COUNT         word36Count
)
{
    std::string tempStr = text;
    tempStr.resize( word36Count * 6, ' ' );
    miscStringToWord36Fieldata( tempStr, pWord36Buffer );
}


//  miscWord36AsciiToString()
//
//  Converts the content of the buffer of Word36 objects into a displayable string,
//  interpreting the content as ASCII characters.
//  The resulting string will always be (word36Count)*4 characters in length.
//  One can presume problems will arise if any of the quarter-words in the source
//  array are zero.
//  Optionally, will convert data so that it is displayable.
std::string
miscWord36AsciiToString
(
    const Word36* const     pWord36Buffer,
    const COUNT             word36Count,
    const bool              formatForDisplay,
    const char              defaultChar
)
{
    const Word36* pWord = pWord36Buffer;
    COUNT wordsLeft = word36Count;
    std::string retStr;
    while ( wordsLeft )
    {
        if ( formatForDisplay )
        {
            retStr += miscAsciiToDisplay( static_cast<char>(pWord->getQ1()), defaultChar );
            retStr += miscAsciiToDisplay( static_cast<char>(pWord->getQ2()), defaultChar );
            retStr += miscAsciiToDisplay( static_cast<char>(pWord->getQ3()), defaultChar );
            retStr += miscAsciiToDisplay( static_cast<char>(pWord->getQ4()), defaultChar );
        }
        else
        {
            retStr += static_cast<char>( pWord->getQ1() );
            retStr += static_cast<char>( pWord->getQ2() );
            retStr += static_cast<char>( pWord->getQ3() );
            retStr += static_cast<char>( pWord->getQ4() );
        }
        ++pWord;
        --wordsLeft;
    }

    return retStr;
}


//  miscWord36FieldataToString()
//
//  Converts the content of the buffer of Word36 objects into a displayable string,
//  interpreting the content as fieldata characters.
//  The resulting string will always be (word36Count)*6 characters in length.
std::string
miscWord36FieldataToString
(
    const Word36* const     pWord36Buffer,
    const COUNT             word36Count
)
{
    const Word36* pWord = pWord36Buffer;
    COUNT wordsLeft = word36Count;
    std::string retStr;
    while ( wordsLeft )
    {
        retStr += miscFieldataToAscii( pWord->getS1() );
        retStr += miscFieldataToAscii( pWord->getS2() );
        retStr += miscFieldataToAscii( pWord->getS3() );
        retStr += miscFieldataToAscii( pWord->getS4() );
        retStr += miscFieldataToAscii( pWord->getS5() );
        retStr += miscFieldataToAscii( pWord->getS6() );
        ++pWord;
        --wordsLeft;
    }

    return retStr;
}


//  miscWord36Pack()
//
//  Packs WORD36 structs into a buffer of BYTEs
//
//  Parameters:
//      pByteBuffer:        pointer to target BYTE buffer
//      pWord36Buffer:      pointer to source Word36 buffer
//      word36Count:        (even) number of Word36's to be packed
void
miscWord36Pack
(
    BYTE* const             pByteBuffer,
    const Word36* const     pWord36Buffer,
    const COUNT             word36Count
)
{
    BYTE* pb = pByteBuffer;
    const Word36* pw = pWord36Buffer;
    for ( INDEX cx = 0; cx < word36Count; cx += 2 )
    {
        *pb = static_cast<BYTE>((pw->getW() >> 28) & 0xff);
        pb++;
        *pb = static_cast<BYTE>((pw->getW() >> 20) & 0xff);
        pb++;
        *pb = static_cast<BYTE>((pw->getW() >> 12) & 0xff);
        pb++;
        *pb = static_cast<BYTE>((pw->getW() >> 4) & 0xff);
        pb++;

        *pb = static_cast<BYTE>((pw->getW() & 0x0f) << 4);
        pw++;
        *pb |= static_cast<BYTE>((pw->getW() >> 32) & 0x0f);
        pb++;

        *pb = static_cast<BYTE>((pw->getW() >> 24) & 0xff);
        pb++;
        *pb = static_cast<BYTE>((pw->getW() >> 16) & 0xff);
        pb++;
        *pb = static_cast<BYTE>((pw->getW() >> 8) & 0xff);
        pb++;
        *pb = static_cast<BYTE>(pw->getW() & 0xff);
        pb++;
        pw++;
    }
}


//  miscWord36Unpack()
//
//  Unpacks Word36 objects from a buffer of BYTE*s
//
//  Parameters:
//      pWord36Buffer:      pointer to target Word36 buffer
//      pByteBuffer:        pointer to source BYTE* buffer
//      word36Count:        (even) number of Word36 objects to be unpacked
void
miscWord36Unpack
(
    Word36* const           pWord36Buffer,
    const BYTE* const       pByteBuffer,
    const COUNT             word36Count
)
{
    const BYTE* pb = pByteBuffer;
    Word36* pw = pWord36Buffer;
    for ( INDEX cx = 0; cx < word36Count; cx += 2 )
    {
        UINT64 value = (static_cast<UINT64>(pb[0]) << 28);
        value |= pb[1] << 20;
        value |= pb[2] << 12;
        value |= pb[3] << 4;
        value |= pb[4] >> 4;
        pw[0].setW( value );

        value = static_cast<UINT64>(pb[4] & 0x0f) << 32;
        value |= (pb[5] << 24) & 0xff000000;
        value |= pb[6] << 16;
        value |= pb[7] << 8;
        value |= pb[8];
        pw[1].setW( value );

        pw += 2;
        pb += 9;
    }
}


//  miscWriteBufferToLog()
//
//  Writes the given BYTE buffer to the system log
void
miscWriteBufferToLog
(
    const std::string&      identifier,
    const std::string&      caption,
    const BYTE* const       pBuffer,
    const COUNT             bytes
)
{
    std::stringstream strm;
    std::stringstream hexStream;
    std::stringstream asciiStream;

    strm << identifier << " -- " << caption;
    SystemLog::write( strm.str() );

    for ( INDEX bx = 0; bx < bytes; bx += 16 )
    {
        strm.str( "" );
        hexStream.str( "" );
        asciiStream.str( "" );

        strm << identifier << ":";
        for ( INDEX by = 0; by < 16; ++by )
        {
            BYTE b = pBuffer[bx + by];
            hexStream << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<unsigned int>(b);
            if ( b < 0x20 || b > 0x7e )
                b = '.';
            asciiStream << static_cast<char>(b);

            if ( (by & 0x03) == 0x03 )
            {
                hexStream << " ";
                asciiStream << " ";
            }
        }

        strm << hexStream.str() << " " << asciiStream.str();
        SystemLog::write( strm.str() );
    }
}


