//  execlib header file
//  All execlib clients should include this, and only this header file (from this library, I mean)
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EXECLIB_H
#define     EXECLIB_H



#include    "../misclib/misclib.h"
#include    "../hardwarelib/hardwarelib.h"

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



//  dump type bit masks
typedef     UINT64                              DUMPBITS;

#define     DUMP_CACHED_DIRECTORY_SECTOR_DATA   0x0001
#define     DUMP_FILE_ALLOCATION_TABLE          0x0002
#define     DUMP_TRACK_ID_MAP                   0x0004
#define     DUMP_MFD_DIRECTORY                  0x0008
#define     DUMP_SEARCH_ITEM_LOOKUP_TABLE       0x0010
#define     DUMP_DISK_ALLOCATIONS               0x0020
#define     DUMP_NORMAL                         ( DUMP_CACHED_DIRECTORY_SECTOR_DATA | DUMP_FILE_ALLOCATION_TABLE | DUMP_SEARCH_ITEM_LOOKUP_TABLE )
#define     DUMP_ALL                            0xFFFFFFFF



//  These are really owned by MFDManager, but at least one of them (LDATINDEX) is required by a module
//  which is included by MFDManager, so we have to establish the typedef here to avoid include loops.
//  So... just to avoid future nastiness, we put all of these out here.
typedef     UINT32              DSADDR;         //  Directory Sector Address (bottom 30 bits of Link Addr)
typedef     UINT64              DRWA;           //  Device-relative word address
typedef     UINT32              LDATINDEX;      //  Identifies a particular fixed or removable pack

typedef     UINT64              EXECTIME;       //  Exec time in usecs since OS2200 epoch



//  Some basic stuff to make life easy

//  More general categorizations of equipment
enum EquipmentCategory
{
    ECAT_DISK,
    ECAT_SYMBIONT,
    ECAT_TAPE,
};


// data density specification -
// various tape device types may use only a subset of these values
enum    TapeDensity
{
    TDENS_800,
    TDENS_1600,
    TDENS_3800,
    TDENS_6250,
    TDENS_38000,
    TDENS_76000,
    TDENS_85937,
    TDENS_5090,
};

// parity - specification -
// various tape device types may use only a subset of these values
enum    TapeParity
{
    TPAR_NONE,
    TPAR_EVEN,
    TPAR_ODD,
};

//  Determines whether disk space is allocated by tracks or by positions
enum    MassStorageGranularity
{
    MSGRAN_TRACK,
    MSGRAN_POSITION,
};

//  Cart option - I doubt we'll use this
enum    TapeBlockNumbering
{
    TBN_OFF,
    TBN_ON,
    TBN_OPTIONAL,
};

//  Another cart option - probably won't use this
enum    TapeDataCompression
{
    TDC_OFF,
    TDC_ON,
    TDC_OPTIONAL,
};

//  This it a more granularized version of IOXLAT_FORMAT.
//  The correspondence is one-to-one.
enum    TapeFormat
{
    TFMT_QUARTER_WORD,
    TFMT_SIX_BIT_PACKED,
    TFMT_EIGHT_BIT_PACKED,
    TFMT_QUARTER_WORD_IGNORE,
};

//  TODO:TAPE Where is this used?
enum    TapeType
{
    TTYPE_SEVEN_TRACK,  //  Other MTAPOP bits clear (i.e., none of the following), and format & 040 is clear
    TTYPE_NINE_TRACK,   //  Other MTAPOP bits clear (i.e., none of the following), and format & 040 is set
    TTYPE_QIC,          //  Quarter-inch cartridge tape: MTAPOP 020
    TTYPE_HIC,          //  Half-inch cartridge tape: MTAPOP 010
    TTYPE_DLT,          //  DLT Cartridge tape: MTAPOP 002
    TTYPE_HIS,          //  Half-inch serpentine tape: MTAPOP 001
};

//  Describes a particular model of equipment - mostly used in Facilities
class   EquipmentModel
{
public:
    SuperString             m_pModelName;           //  up to 6-character equipment name
    EquipmentCategory       m_EquipmentCategory;    //  very general categorization
    EquipmentCode           m_EquipmentCode;        //  more finely-resolved category

    EquipmentModel( const char* const   pName,
                    EquipmentCategory   category,
                    EquipmentCode       code )
        :m_pModelName( pName ),
        m_EquipmentCategory( category ),
        m_EquipmentCode( code )
    {}
};



#include    "Activity.h"
#include        "ExtrinsicActivity.h"
#include        "IntrinsicActivity.h"
#include            "BootActivity.h"
#include            "CoarseSchedulerActivity.h"
#include            "ConsoleActivity.h"
#include            "ControlStatementActivity.h"
#include            "IoActivity.h"
#include            "KeyinActivity.h"
#include                "CSKeyin.h"
#include                "DollarBangKeyin.h"
#include                "DUKeyin.h"
#include                "FacilitiesKeyin.h"
#include                    "DNKeyin.h"
#include                    "FSKeyin.h"
#include                    "RVKeyin.h"
#include                    "SUKeyin.h"
#include                    "UPKeyin.h"
#include                "FFKeyin.h"
#include                "JumpKeyKeyin.h"
#include                    "CJKeyin.h"
#include                    "DJKeyin.h"
#include                    "SJKeyin.h"
#include                "MSKeyin.h"
#include                "PREPKeyin.h"
#include                "SSKeyin.h"
#include            "PollActivity.h"
#include            "RSIActivity.h"
#include            "TransparentActivity.h"
#include    "Configuration.h"
#include    "ConsoleInterface.h"
#include    "CSInterpreter.h"
#include    "DiskAllocationTable.h"
#include    "Exec.h"
#include    "ExecManager.h"
#include        "AccountManager.h"
#include        "ConsoleManager.h"
#include        "DeviceManager.h"
#include        "FacilitiesManager.h"
#include        "IoManager.h"
#include        "MFDManager.h"
#include        "QueueManager.h"
#include        "RSIManager.h"
#include        "SecurityManager.h"
#include    "FacilityItem.h"
#include        "NonStandardFacilityItem.h"
#include        "StandardFacilityItem.h"
#include            "DiskFacilityItem.h"
#include            "TapeFacilityItem.h"
#include    "FileAllocationTable.h"
#include    "FileSpecification.h"
#include    "MasterConfigurationTable.h"
#include    "NodeTable.h"
#include    "PanelInterface.h"
#include    "RunConditionWord.h"
#include    "RunInfo.h"
#include        "ExecRunInfo.h"
#include        "UserRunInfo.h"
#include            "ControlModeRunInfo.h"
#include                "BatchRunInfo.h"
#include                "DemandRunInfo.h"
#include            "TIPRunInfo.h"
#include    "SecurityContext.h"
#include    "SymbiontBuffer.h"
#include    "Task.h"
#include    "TransparentCSInterpreter.h"



int             execCompareAbsoluteCycles( const UINT16 cycle1, const UINT16 cycle2 );
UINT16          execDecrementAbsoluteCycle( const UINT16 cycle, const UINT16 value );
UINT16          execGetAbsoluteCycleRange( const UINT16 cycle1, const UINT16 cycle2 );
const char*     execGetGranularityString( const MassStorageGranularity granularity );
ChannelModule::IoTranslateFormat    execGetIoTranslateFormat( const TapeFormat tapeFormat );
std::string     execGetOptionsString( const UINT32 options );
const char*     execGetTapeDataCompressionString( const TapeDataCompression compression );
const char*     execGetTapeDensityString( const TapeDensity density );
const char*     execGetTapeFormatString( const TapeFormat format );
const char*     execGetTapeParityString( const TapeParity parity );
const char*     execGetTapeTypeString( const TapeType type );
UINT16          execIncrementAbsoluteCycle( const UINT16 cycle, const UINT16 value );



#endif

