//	main header file for misclib



#ifndef	EM2200_MISCLIB_H
#define	EM2200_MISCLIB_H



#ifdef WIN32
#pragma warning( disable : 4127 4510 4512 4996 )
#endif



#include    <assert.h>
#include    <stdint.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <time.h>

#ifdef  WIN32
#include    <process.h>
#include    <WinSock2.h>
#include    <WS2tcpip.h>
#include    <Windows.h>
#else
#include    <arpa/inet.h>
#include    <fcntl.h>
#include    <netdb.h>
#include    <pthread.h>
#include    <string.h>
#include    <sys/poll.h>
#include    <sys/socket.h>
#include    <sys/stat.h>
#include    <sys/sysinfo.h>
#include    <sys/time.h>
#include    <sys/types.h>
#include    <unistd.h>
#endif

#include    <fstream>
#include    <iomanip>
#include    <iostream>
#include    <list>
#include    <map>
#include    <mutex>
#include    <set>
#include    <sstream>
#include    <string>
#include    <vector>



// Project definitions
#define     VERSION     "0.2"
#define     COPYRIGHT   "Copyright (c) 2014-2015 by Kurt Duncan"

//  Paths
#ifdef WIN32
#define     BASE_PATH       "C:\\ProgramData\\em2200\\"
#define     PACKS_PATH      BASE_PATH "packs\\"
#define     VOLUMES_PATH    BASE_PATH "volumes\\"
#else
#define     BASE_PATH       "/opt/em2200/"
#define     PACKS_PATH      BASE_PATH "packs/"
#define     VOLUMES_PATH    BASE_PATH "volumes/"
#endif

// Useful byte definitions
#define     ASCII_NUL       (BYTE)0x00
#define     ASCII_SOH       (BYTE)0x01
#define     ASCII_STX       (BYTE)0x02
#define     ASCII_ETX       (BYTE)0x03
#define     ASCII_EOT       (BYTE)0x04
#define     ASCII_ENQ       (BYTE)0x05
#define     ASCII_ACK       (BYTE)0x06
#define     ASCII_BEL       (BYTE)0x07
#define     ASCII_BS        (BYTE)0x08
#define     ASCII_HT        (BYTE)0x09
#define     ASCII_LF        (BYTE)0x0A
#define     ASCII_VT        (BYTE)0x0B
#define     ASCII_FF        (BYTE)0x0C
#define     ASCII_CR        (BYTE)0x0D
#define     ASCII_SO        (BYTE)0x0E
#define     ASCII_SI        (BYTE)0x0F
#define     ASCII_DLE       (BYTE)0x10
#define     ASCII_DC1       (BYTE)0x11
#define     ASCII_DC2       (BYTE)0x12
#define     ASCII_DC3       (BYTE)0x13
#define     ASCII_DC4       (BYTE)0x14
#define     ASCII_NAK       (BYTE)0x15
#define     ASCII_SYN       (BYTE)0x16
#define     ASCII_ETB       (BYTE)0x17
#define     ASCII_CAN       (BYTE)0x18
#define     ASCII_EM        (BYTE)0x19
#define     ASCII_SUB       (BYTE)0x1A
#define     ASCII_ESC       (BYTE)0x1B
#define     ASCII_FS        (BYTE)0x1C
#define     ASCII_GS        (BYTE)0x1D
#define     ASCII_RS        (BYTE)0x1E
#define     ASCII_US        (BYTE)0x1F
#define     ASCII_WS        (BYTE)0x20
#define     ASCII_SPACE     (BYTE)0x20
#define     ASCII_QUOTE     (BYTE)0x27
#define     ASCII_DEL       (BYTE)0x7F
#define     FD_SPACE        (BYTE)0x05



#ifdef WIN32
typedef DWORD               SYSTEMERRORCODE;
#define SYSTEMERRORCODE_SUCCESS     ERROR_SUCCESS
#else
typedef unsigned char       BYTE;
typedef int                 SYSTEMERRORCODE;
#define SYSTEMERRORCODE_SUCCESS     0
#endif

typedef int8_t              INT8;
typedef int16_t             INT16;
typedef int32_t             INT32;
typedef int64_t             INT64;
typedef uint8_t             UINT8;
typedef uint16_t            UINT16;
typedef uint32_t            UINT32;
typedef uint64_t            UINT64;

typedef size_t              COUNT;
typedef UINT32              COUNT32;
typedef UINT64              COUNT64;
typedef size_t              INDEX;
typedef UINT32              INDEX32;
typedef UINT64              INDEX64;

//  some useful STL-derived things
typedef std::vector<BYTE>               VBYTE;
typedef std::list<std::string>          LSTRING;
typedef LSTRING::iterator               LITSTRING;
typedef LSTRING::const_iterator         LCITSTRING;
typedef std::vector<std::string>        VSTRING;

//  Exec and Univac related stuff
typedef     UINT64          REAL_ADDRESS;
typedef     UINT64          BLOCK_COUNT;
typedef     UINT64          BLOCK_ID;
typedef     UINT32          BLOCK_SIZE;
typedef     UINT32          PREP_FACTOR;
typedef     UINT32          PROCESSOR_UPI;
typedef     UINT64          SECTOR_COUNT;
typedef     UINT64          SECTOR_ID;
typedef     UINT64          TRACK_COUNT;
typedef     UINT64          TRACK_ID;
typedef     UINT64          WORD_COUNT;
typedef     UINT64          WORD_ID;

#define     WORDS_PER_TRACK                 1792
#define     WORDS_PER_SECTOR                28
#define     SECTORS_PER_TRACK               64
#define     SECTORS_PER_BLOCK(prepFactor)   ((prepFactor) / WORDS_PER_SECTOR)
#define     BLOCKS_PER_TRACK(prepFactor)    (WORDS_PER_TRACK / (prepFactor))

//  command option bit masks
#define     OPTB_A              0x02000000
#define     OPTB_B              0x01000000
#define     OPTB_C              0x00800000
#define     OPTB_D              0x00400000
#define     OPTB_E              0x00200000
#define     OPTB_F              0x00100000
#define     OPTB_G              0x00080000
#define     OPTB_H              0x00040000
#define     OPTB_I              0x00020000
#define     OPTB_J              0x00010000
#define     OPTB_K              0x00008000
#define     OPTB_L              0x00004000
#define     OPTB_M              0x00002000
#define     OPTB_N              0x00001000
#define     OPTB_O              0x00000800
#define     OPTB_P              0x00000400
#define     OPTB_Q              0x00000200
#define     OPTB_R              0x00000100
#define     OPTB_S              0x00000080
#define     OPTB_T              0x00000040
#define     OPTB_U              0x00000020
#define     OPTB_V              0x00000010
#define     OPTB_W              0x00000008
#define     OPTB_X              0x00000004
#define     OPTB_Y              0x00000002
#define     OPTB_Z              0x00000001

//  For debugging...
#ifdef  WIN32
#define     STAMP_DEBUG(msg)                \
{                                           \
    time_t tnow = time( 0 );                \
    std::stringstream s;                    \
    s << tnow << ":" << msg << std::endl;   \
    OutputDebugString(s.str().c_str());     \
}
#else
#define STAMP_DEBUG(msg)
#endif


//  Mainly for ER FITEM$
enum EquipmentCode
{
    ECODE_NONE          = 0,
    ECODE_UNISERVO_7    = 014,  //  Uniservo 22D/24D/30D
    ECODE_UNISERVO_9    = 015,  //  Uniservo 26N/28N/32N/34N/36N/45N
    ECODE_VTH           = 016,  //  Virtual Tape Handler VTH
    ECODE_CTAPE         = 017,  //  CT0899/SCTAPE/U47/U47L/U47M/U47LM/U5136/U5236/DLT7000/DVDTP
    ECODE_WORD_DISK     = 024,  //  Word-addressable Mass Storage
    ECODE_SECTOR_DISK   = 036,  //  Sector-addressable Mass Storage
    ECODE_CHANNEL       = 075,  //  HPRDEV,DCPBDV,HLCDEV
    ECODE_ARBDEV        = 077,  //  ARBDEV/AC40/CRYPDV/CTLDEV
};

//  'G' field in Exec IO packet
enum    ExecIoBufferAddressModifier
{
    EXIOBAM_INCREMENT                   = 0,
    EXIOBAM_NO_CHANGE                   = 1,
    EXIOBAM_DECREMENT                   = 2,
    EXIOBAM_SKIP_DATA                   = 3,
};

//  Function field in Exec IO packet
enum    ExecIoFunction
{
    EXIOFUNC_WRITE_BY_BDI               = 004,
    EXIOFUNC_BLOCK_ID_SAFE              = 005,
    EXIOFUNC_READ_BLOCK_ID_BEFORE_WRITE = 006,
    EXIOFUNC_WRITE_BY_BDI_EXTENDED      = 007,
    EXIOFUNC_WRITE                      = 010,
    EXIOFUNC_WRITE_END_OF_FILE          = 011,
    EXIOFUNC_WRITE_ADDRESS_AND_LENGTH   = 013,
    EXIOFUNC_GATHER_WRITE               = 015,
    EXIOFUNC_ACQUIRE                    = 016,
    EXIOFUNC_EXTENDED_ACQUIRE           = 017,
    EXIOFUNC_READ                       = 020,
    EXIOFUNC_READ_BACKWARD              = 021,
    EXIOFUNC_READ_AND_RELEASE           = 022,
    EXIOFUNC_RELEASE                    = 023,
    EXIOFUNC_BLOCK_READ_DRUM            = 024,
    EXIOFUNC_READ_AND_LOCK              = 025,
    EXIOFUNC_UNLOCK                     = 026,
    EXIOFUNC_EXTENDED_RELEASE           = 027,
    EXIOFUNC_TRACK_SEARCH_ALL           = 030,
    EXIOFUNC_TRACK_SEARCH_FIRST         = 031,
    EXIOFUNC_POSITION_SEARCH_ALL        = 032,
    EXIOFUNC_POSITION_SEARCH_FIRST      = 033,
    EXIOFUNC_SEARCH_DRUM                = 034,
    EXIOFUNC_BLOCK_SEARCH_DRUM          = 035,
    EXIOFUNC_SEARCH_READ_DRUM           = 036,
    EXIOFUNC_BLOCK_SEARCH_READ_DRUM     = 037,
    EXIOFUNC_REWIND                     = 040,
    EXIOFUNC_REWIND_WITH_INTERLOCK      = 041,
    EXIOFUNC_SET_MODE                   = 042,
    EXIOFUNC_SCATTER_READ               = 043,
    EXIOFUNC_SCATTER_READ_BACKWARD      = 044,
    EXIOFUNC_READ_BLOCK_IDENTIFIER      = 045,
    EXIOFUNC_LOCATE_BLOCK               = 046,
    EXIOFUNC_READ_BY_BDI_EXTENDED       = 047,
    EXIOFUNC_MOVE_FORWARD               = 050,
    EXIOFUNC_MOVE_BACKWARD              = 051,
    EXIOFUNC_FORWARD_SPACE_FILE         = 052,
    EXIOFUNC_BACK_SPACE_FILE            = 053,
    EXIOFUNC_READ_BY_BDI                = 054,
    EXIOFUNC_MODE_SET                   = 055,
    EXIOFUNC_READ_BACKWARD_BY_BDI       = 056,
    EXIOFUNC_FEP_INITIALIZATION         = 060,
    EXIOFUNC_FEP_TERMINATION            = 061,
    EXIOFUNC_SENSE_STATISTICS           = 062,
    EXIOFUNC_END                        = 063,
    EXIOFUNC_SET_TEST                   = 064,
    EXIOFUNC_MULTIREQUEST               = 065,
    EXIOFUNC_INPUT_DATA                 = 066,
    EXIOFUNC_FILE_UPDATE_WAIT           = 073,
    EXIOFUNC_LOAD_CODE_CONV_BANK        = 077,
};

//  status field for Exec IO packet
enum    ExecIoStatus
    //  X - implemented for indicated device type
    //  * - potentially implemented for indicated device type
    //    - not returned for indicated device type
    //  Codes 000 through 017 and 040 do not generate contingencies.
    //  The others generate Contingency Type 012, Error Type 01 (I/O)
{                                               //  Disk    Tape    FEH     Arb
    EXIOSTAT_SUCCESSFUL                 = 000,  //   *       *       *       *
    EXIOSTAT_END_OF_BLOCK               = 001,  //   *                          //  Block read/block search read truncated by end-of-block
    EXIOSTAT_END_OF_FILE                = 001,  //           *                  //  End of file (tape mark) detected
    EXIOSTAT_UNIT_EXCEPTION             = 001,  //                   *
    EXIOSTAT_TIMEOUT                    = 001,  //                           *
    EXIOSTAT_EACQ_FAILED                = 002,  //   *                          //  See substatus
    EXIOSTAT_END_OF_TAPE                = 002,  //           *                  //  End of tape on read-backward from load point,
                                                                            //      or on write
    EXIOSTAT_BLOCK_READ_TRUNCATED       = 003,  //   *                          //  No find on block read/block search read
    EXIOSTAT_INVALID_BLOCK_ID           = 003,  //           *                  //  No find on LBLK$
    EXIOSTAT_WAIT_REQS_CANCELED         = 003,  //                           *
    EXIOSTAT_NON_INTEGRAL_BLOCK         = 004,  //           *
    EXIOSTAT_REGION_NOT_ALLOCATED       = 005,  //   *                          //  Read request only
    EXIOSTAT_EXPANSION_QUOTA_LIMIT      = 006,  //   *                          //  File expansion limited by quota
    EXIOSTAT_WRITE_TOO_LARGE            = 006,  //           *                  //  Attempt to write > 131K block to tape
    EXIOSTAT_INTERNAL_ERROR             = 007,  //   *       *       *       *
    EXIOSTAT_UNLOCK_TIMEOUT             = 010,  //   *                          //  Write or unlock request
    EXIOSTAT_NON_RECOVERABLE_ERROR      = 011,  //   *       *       *       *
    EXIOSTAT_BAD_SPOT                   = 012,  //   *                          //  B answer to console message, or granule bad-spotted
    EXIOSTAT_LOSS_OF_POSITION           = 012,  //           *                  //  B answer for tape, loss of position occurred
    EXIOSTAT_DEVICE_NOT_AVAILABLE       = 013,  //   *       *       *       *  //  No path to device
    EXIOSTAT_PACK_UNAVAILABLE           = 015,  //   *                          //  Pack is DN or has inhibits set
    EXIOSTAT_NO_SYSTEM_RESOURCES        = 017,  //   *       *       *       *
    EXIOSTAT_INHIBITED                  = 020,  //   *       *       *       *  //  Write function attempted on write inhibited file,
                                                                                //  Read function attempted on read inhibited file,
                                                                                //  ACQ$,RR$,REL$,RDL$,UNL$ attempted write inhibited file
    EXIOSTAT_FILE_NOT_ASSIGNED          = 021,  //   *       *       *       *
    EXIOSTAT_ADDRESS_BEYOND_MAXIMUM     = 022,  //   *                          //  Requested IO range exceeds max allowed
    EXIOSTAT_MASS_STORAGE_OVERFLOW      = 022,  //   *                          //  Fixed storage exhausted
    EXIOSTAT_INVALID_FUNCTION_CODE      = 024,  //   *       *       *       *
    EXIOSTAT_INCOMPATIBLE_MODE_FIELD    = 024,  //           *                  //  Incompatible field on mode set or set mode
    EXIOSTAT_PRIVILEGED_FUNCTION        = 024,  //   *       *       *       *
    EXIOSTAT_WRITE_LESS_THAN_NOISE      = 025,  //           *                  //  Attempt to write tape block smaller than noise constant
    EXIOSTAT_PACKET_IN_USE              = 027,  //   *       *       *       *
    EXIOSTAT_TASK_ABORT                 = 033,  //   *       *       *       *  //  Operator terminated the task
    EXIOSTAT_ADDRESS_TRANSLATION        = 034,  //   *                          //  Internal conversion of file-relative address failed
    EXIOSTAT_BAD_DEVICE_ADDRESS         = 034,  //   *                          //  Illegal device-relative address specified
    EXIOSTAT_ALREADY_LOCKED             = 035,  //   *                          //  Lock request indicated already-locked region
    EXIOSTAT_ALREADY_UNLOCKED           = 035,  //   *                          //  Unlock request indicated non-locked region
    EXIOSTAT_NO_OUTSTANDING_IOS         = 036,  //   *       *       *       *  //  For WAIT$, WANY$, etc
    EXIOSTAT_IN_PROGRESS                = 040,  //   *       *       *       *
};


//  some basic stuff that the included files might need

UINT16          miscAdd12( const UINT16     operand1,
                            const UINT16    operand2,
                            bool* const     pCarry,
                            bool* const     pOverflow );
UINT32          miscAdd18( const UINT32     operand1,
                            const UINT32    operand2,
                            bool* const     pCarry,
                            bool* const     pOverflow );
UINT32          miscAdd24( const UINT32     operand1,
                            const UINT32    operand2,
                            bool* const     pCarry,
                            bool* const     pOverflow );
UINT64          miscAdd36( const UINT64     operand1,
                            const UINT64    operand2,
                            bool* const     pCarry,
                            bool* const     pOverflow );

inline UINT16   miscComplement12( const UINT16 operand )        { return operand ^ 07777; }
inline UINT32   miscComplement18( const UINT32 operand )        { return operand ^ 0777777; }
inline UINT32   miscComplement24( const UINT32 operand )        { return operand ^ 077777777; }
inline UINT64   miscComplement36( const UINT64 operand )        { return operand ^ 0777777777777; }

inline bool     miscIsNegative12( const UINT16 operand )        { return ( operand & 04000 ) != 0; }
inline bool     miscIsNegative18( const UINT32 operand )        { return ( operand & 0400000 ) != 0; }
inline bool     miscIsNegative24( const UINT32 operand )        { return ( operand & 040000000 ) != 0; }
inline bool     miscIsNegative36( const UINT64 operand )        { return ( operand & 0400000000000 ) != 0; }

inline UINT16   miscOnesComplement12( const INT16 operand )     { return (operand < 0) ? miscComplement12( -operand ) : operand; }
inline UINT32   miscOnesComplement18( const INT32 operand )     { return (operand < 0) ? miscComplement18( -operand ) : operand; }
inline UINT32   miscOnesComplement24( const INT32 operand )     { return (operand < 0) ? miscComplement24( -operand ) : operand; }
inline UINT64   miscOnesComplement36( const INT64 operand )     { return (operand < 0) ? miscComplement36( -operand ) : operand; }

inline UINT64   miscSignExtend12( const UINT16 operand )        { return miscIsNegative12( operand ) ? (static_cast<UINT64>(operand) | 0777777770000) : operand; }
inline UINT32   miscSignExtend12To24( const UINT16 operand )    { return miscIsNegative12( operand ) ? (static_cast<UINT64>(operand) | 077770000) : operand; }
inline UINT64   miscSignExtend18( const UINT32 operand )        { return miscIsNegative18( operand ) ? (static_cast<UINT64>(operand) | 0777777000000) : operand; }
inline UINT64   miscSignExtend24( const UINT32 operand )        { return miscIsNegative24( operand ) ? (static_cast<UINT64>(operand) | 0777700000000) : operand; }

inline UINT16   miscSubtract12( const UINT16    minuend,
                                const UINT16    subtrahend,
                                bool* const     pCarry,
                                bool* const     pOverflow )     { return miscAdd12( minuend, miscOnesComplement12( subtrahend ), pCarry, pOverflow ); }
inline UINT32   miscSubtract18( const UINT32    minuend,
                                const UINT32    subtrahend,
                                bool* const     pCarry,
                                bool* const     pOverflow )     { return miscAdd18( minuend, miscOnesComplement18( subtrahend ), pCarry, pOverflow ); }
inline UINT32   miscSubtract24( const UINT32    minuend,
                                const UINT32    subtrahend,
                                bool* const     pCarry,
                                bool* const     pOverflow )     { return miscAdd24( minuend, miscOnesComplement24( subtrahend ), pCarry, pOverflow ); }
inline UINT64   miscSubtract36( const UINT64    minuend,
                                const UINT64    subtrahend,
                                bool* const     pCarry,
                                bool* const     pOverflow )     { return miscAdd36( minuend, miscOnesComplement36( subtrahend ), pCarry, pOverflow ); }

inline INT16    miscTwosComplement12( const UINT16 operand )    { return miscIsNegative12( operand ) ? (0 - miscComplement12( operand )) : operand; }
inline INT32    miscTwosComplement18( const UINT32 operand )    { return miscIsNegative18( operand ) ? (0 - miscComplement18( operand )) : operand; }
inline INT32    miscTwosComplement24( const UINT32 operand )    { return miscIsNegative24( operand ) ? (0 - miscComplement24( operand )) : operand; }
inline INT64    miscTwosComplement36( const UINT64 operand )    { return miscIsNegative36( operand ) ? (0 - miscComplement36( operand )) : operand; }



#include    "DataHandler.h"         //???? is this obsolete?
#include    "Emitter.h"
#include    "Event.h"
#include    "GeneralRegister.h"     //???? should this be in hardwarelib?
#include    "HttpServer.h"
#include    "InstructionWord.h"     //????  should this be in hardwarelib?
#include    "Listener.h"
#include    "Lockable.h"
#include    "NetServer.h"
#include    "SimpleFile.h"
#include    "SuperString.h"
#include    "SystemLog.h"
#include    "SystemTime.h"
#include    "TDate.h"
#include    "Word36.h"
#include    "Worker.h"



//  static functions
char            miscAsciiToDisplay( const char  ascChar,
                                    const char  defaultChar = ' ' );
void            miscDumpWord36Buffer( std::ostream&         stream,
                                      const Word36* const   pBuffer,
                                      const COUNT           wordCount );
void            miscDumpWord36BufferToLog( SystemLog* const     pSystemLog,
                                           const Word36* const  pBuffer,
                                           const COUNT          wordCount );
char            miscFieldataToAscii( const char fdChar );
COUNT64         miscGetAvailableMemory();
BLOCK_SIZE      miscGetBlockSizeFromPrepFactor( PREP_FACTOR prepFactor );
SYSTEMERRORCODE miscGetErrorCode();
std::string     miscGetErrorCodeString( const SYSTEMERRORCODE errorCode );
const char*     miscGetExecIoBufferAddressModifierString( const ExecIoBufferAddressModifier value );
const char*     miscGetExecIoFunctionString( const ExecIoFunction value );
PREP_FACTOR     miscGetPrepFactorFromBlockSize( BLOCK_SIZE blockSize );
COUNT64         miscGetTotalMemory();
bool            miscIsValidBlockSize( const BLOCK_SIZE blockSize );
bool            miscIsValidNodeName( const std::string& packName );
bool            miscIsValidPackName( const std::string& packName );
bool            miscIsValidPrepFactor( const PREP_FACTOR prepFactor );
UINT64          miscOnesCompAdd36( const UINT64&    operand1,
                                    const UINT64&   operand2,
                                    bool* const     pCarry,
                                    bool* const     pOverflow );
UINT64          miscOnesCompSubtract36( const UINT64&   operand1,
                                        const UINT64&   operand2,
                                        bool* const     pCarry,
                                        bool* const     pOverflow );
void            miscSleep( const COUNT32 milliseconds );
void            miscStringToWord36Ascii( const std::string& text,
                                            Word36* const   pWord36Buffer );
void            miscStringToWord36Ascii( const std::string& text,
                                            Word36* const   pWord36Buffer,
                                            const COUNT     word36Count );
void            miscStringToWord36Fieldata( const std::string&  text,
                                            Word36* const       pWord36Buffer );
void            miscStringToWord36Fieldata( const std::string&  text,
                                            Word36* const       pWord36Buffer,
                                            const COUNT         word36Count );
void            miscWord36Pack( BYTE* const         pByteBuffer,
                                const Word36* const pWord36Buffer,
                                const COUNT         word36Count );
std::string     miscWord36AsciiToString( const Word36* const    pWord36Buffer,
                                            const COUNT         word36Count,
                                            const bool          formatForDisplay,
                                            const char          defaultChar = ' ' );
std::string     miscWord36FieldataToString( const Word36* const pWord36Buffer,
                                            const COUNT         word36Count );
void            miscWord36Unpack( Word36* const         pWord36Buffer,
                                    const BYTE* const   pByteBuffer,
                                    const COUNT         word36Count );
void            miscWriteBufferToLog( const std::string&    identifer,
                                        const std::string&  caption,
                                        const BYTE* const   pBuffer,
                                        const COUNT         bytes );



#endif
