//  Facilities class implementation
//  Copyright (c) 2015 by Kurt Duncan



//TODO:ER @ASG and @USE must return a file-index, which can be used for subsequent requests (see ER PRM 7.3.2 'file index')

//TODO:ABSASG  If we ever support absolute assignment, then we'd need to update checkEquipmentType... ?
//TODO:ABSASG  E:240233 Absolute assignment of tape not allowed with CY or UY options.
//TODO:ABSASG  E:240333 Illegal options on absolute assignment.
//TODO:ABSASG  E:242333 Assignment of units of the requested equipment type is not allowed.
//TODO:ABSASG  E:246733 Logical channel not allowed with absolute device assign
//TODO:ABSASG  E:250233 Number of units not allowed with absolute device assign.
//TODO:TAPE  E:254033 Ring specification not allowed on CAT image.
//TODO:TAPE  E:256333 Units and logical subfields not allowed on CAT image.



#include "execlib.h"



//  Locating items in the additionalFields object -
//  FIELD is relative to additional fields (i.e., 0 is the first field after the file-name field)
#define     EQUIPMENT_TYPE_FIELD                0
#define     EQUIPMENT_TYPE_SUBFIELD             0

#define     DISK_INITIAL_RESERVE_FIELD          0
#define     DISK_INITIAL_RESERVE_SUBFIELD       1
#define     DISK_GRANULARITY_FIELD              0
#define     DISK_GRANULARITY_SUBFIELD           2
#define     DISK_MAX_GRANULES_FIELD             0
#define     DISK_MAX_GRANULES_SUBFIELD          3
#define     DISK_PLACEMENT_FIELD                0
#define     DISK_PLACEMENT_SUBFIELD             4

#define     TAPE_UNITS_FIELD                    0
#define     TAPE_UNITS_SUBFIELD                 0
#define     TAPE_LOGICAL_FIELD                  0
#define     TAPE_LOGICAL_SUBFIELD               1
#define     TAPE_NOISE_FIELD                    0
#define     TAPE_NOISE_SUBFIELD                 2
#define     TAPE_PROCESSOR_FIELD                0
#define     TAPE_PROCESSOR_SUBFIELD             3
#define     TAPE_TAPE_FIELD                     0
#define     TAPE_TAPE_SUBFIELD                  4
#define     TAPE_FORMAT_FIELD                   0
#define     TAPE_FORMAT_SUBFIELD                5
#define     TAPE_DATA_CONVERTER_FIELD           0
#define     TAPE_DATA_CONVERTER_SUBFIELD        6
#define     TAPE_BLOCK_NUMBERING_FIELD          0
#define     TAPE_BLOCK_NUMBERING_SUBFIELD       7
#define     TAPE_DATA_COMPRESSION_FIELD         0
#define     TAPE_DATA_COMPRESSION_SUBFIELD      8
#define     TAPE_EXPANDED_BUFFER_FIELD          0
#define     TAPE_EXPANDED_BUFFER_SUBFIELD       9
#define     TAPE_REEL_NUMBER_FIELD              1
#define     TAPE_EXPIRATION_FIELD               2
#define     TAPE_EXPIRATION_SUBFIELD            0
#define     TAPE_MEDIA_MANAGER_FIELD            2
#define     TAPE_MEDIA_MANAGER_SUBFIELD         1
#define     TAPE_RING_INDICATOR_FIELD           3
#define     TAPE_RING_INDICATOR_SUBFIELD        0
#define     TAPE_ACR_NAME_FIELD                 4
#define     TAPE_ACR_NAME_SUBFIELD              0
#define     TAPE_CARTRIDGE_POOL_FIELD           5
#define     TAPE_CARTRIDGE_POOL_SUBFIELD        0



//  statics

const FacilitiesManager::EquipmentType*     FacilitiesManager::m_pDefaultEquipMassStorage;
const FacilitiesManager::EquipmentType*     FacilitiesManager::m_pDefaultEquipTape;
const FacilitiesManager::EquipmentType*     FacilitiesManager::m_pDefaultEquipWordAddressable;

//  Static EquipmentModel objects, followed by the static tables which are loaded in the construtor
EquipmentModel ModelEMFSD( "EMFSD", ECAT_DISK, ECODE_SECTOR_DISK );
EquipmentModel ModelEMFST( "EMFST", ECAT_TAPE, ECODE_UNISERVO_9 );

FacilitiesManager::StatusCodeInfo FacilitiesManager::m_StatusCodeInfoTable[] =
{
    //  Infos
    {FSCODE_HELD_FOR_UNIT_FOR_ABS_DEVICE,                   FSSCAT_INFO, "Run % held for unit for abs device/CU unit assign for %  min.",    true,   FSCODE_HOLD_FOR_DEV_CU_REJECTED_Z},
    {FSCODE_HELD_FOR_UNIT_FOR_ABS_REM_DISK,                 FSSCAT_INFO, "Run % held for unit for abs rem disk for % min.",                  true,   FSCODE_HOLD_FOR_REM_DISK_REJECTED_Z},
    {FSCODE_HELD_FOR_PACK,                                  FSSCAT_INFO, "Run % held for pack availability for % min.",                      true,   FSCODE_HOLD_FOR_PACK_REJECTED_Z},
    {FSCODE_HELD_FOR_REEL,                                  FSSCAT_INFO, "Run % held for reel availability for % min.",                      true,   FSCODE_HOLD_FOR_REEL_REJECTED_Z},
    {FSCODE_HELD_FOR_COM_LINE,                              FSSCAT_INFO, "Run % held for com line availability for % min.",                  true,   FSCODE_HOLD_FOR_COM_LINE_REJECTED_Z},
    {FSCODE_HELD_FOR_COM_GROUP,                             FSSCAT_INFO, "Run % held for com group availability for % min.",                 true,   FSCODE_HOLD_FOR_COM_GROUP_REJECTED_Z},
    {FSCODE_HELD_FOR_MASS_STORAGE_SPACE,                    FSSCAT_INFO, "Run % held for mass storage space for % min.",                     true,   FSCODE_HOLD_FOR_MASS_STORAGE_SPACE_REJECTED_Z},
    {FSCODE_HELD_FOR_TAPE_UNIT,                             FSSCAT_INFO, "Run % held for tape unit availability for % min.",                 true,   FSCODE_HOLD_FOR_TAPE_UNIT_REJECTED_Z},
    {FSCODE_HELD_FOR_EXCLUSIVE_FILE_USE_RELEASE,            FSSCAT_INFO, "Run % held for exclusive file use release for % min.",             true,   FSCODE_HOLD_FOR_X_USE_REJECTED_Z},
    {FSCODE_HELD_FOR_NEED_OF_EXCLUSIVE_USE,                 FSSCAT_INFO, "Run % held for need of exclusive use for % min.",                  true,   FSCODE_HOLD_FOR_RELEASE_OF_X_USE_REJECTED_Z},
    {FSCODE_HELD_FOR_DISK_UNIT,                             FSSCAT_INFO, "Run % held for disk unit availability for % min.",                 true,   FSCODE_HOLD_FOR_DISK_UNIT_REJECTED_Z},
    {FSCODE_HELD_FOR_ROLLBACK,                              FSSCAT_INFO, "Run % held for rollback of unloaded file for % min.",              true,   FSCODE_HOLD_FOR_ROLLBACK_REJECTED_Z},
    {FSCODE_HELD_FOR_FILE_CYCLE_CONFLICT,                   FSSCAT_INFO, "Run % held for file cycle conflict for % min.",                    true,   FSCODE_HOLD_FOR_FCYCLE_CONFLICT_REJECTED_Z},
    {FSCODE_HELD_FOR_DISK_PACK,                             FSSCAT_INFO, "Run % held for disk pack to be mounted for % min.",                true,   FSCODE_HOLD_FOR_PACK_REJECTED_Z},
    {FSCODE_DEVICE_IS_SELECTED,                             FSSCAT_INFO, "% is selected % % %",                                              false,  FSCODE_NONE},
    {FSCODE_HELD_FOR_CONTROL_OF_CACHING,                    FSSCAT_INFO, "Run % held for control of caching for % min.",                     true,   FSCODE_HOLD_FOR_CONTROL_OF_CACHING_REJECTED_Z},
    {FSCODE_HELD_FOR_DISKETTE_UNIT,                         FSSCAT_INFO, "Run % held for diskette unit availability for % min.",             true,   FSCODE_HOLD_FOR_DISKETTE_UNIT_REJECTED_Z},
    {FSCODE_HELD_FOR_DISKETTE,                              FSSCAT_INFO, "Run % held for diskette to be mounted for % min.",                 true,   FSCODE_HOLD_FOR_DISKETTE_UNIT_REJECTED_Z},
    {FSCODE_COMPLETE,                                       FSSCAT_INFO, "% Complete.",                                                      false,  FSCODE_NONE},

    //  Warnings
    {FSCODE_FILE_IS_ALREADY_ASSIGNED,                       FSSCAT_WARNING, "File is already assigned.",                                        false,  FSCODE_NONE},
    {FSCODE_FILE_IS_ALREADY_ASSIGNED_THIS_IMAGE_IGNORED,    FSSCAT_WARNING, "File is already assigned, this image ignored.",                    false,  FSCODE_NONE},
    {FSCODE_FILE_IS_ASSIGNED_TO_ANOTHER_RUN,                FSSCAT_WARNING, "File is assigned to another run.",                                 false,  FSCODE_NONE},
    {FSCODE_FILE_NAME_NOT_KNOWN_TO_THIS_RUN,                FSSCAT_WARNING, "Filename not known to this run.",                                  false,  FSCODE_NONE},
    {FSCODE_FILE_NAME_NOT_UNIQUE,                           FSSCAT_WARNING, "File name is not unique.",                                         false,  FSCODE_NONE},
    {FSCODE_MFD_IO_ERROR_DURING_FREE,                       FSSCAT_WARNING, "I/O error encountered on MFD during FREE.",                        false,  FSCODE_NONE},
    {FSCODE_PLACEMENT_FIELD_IGNORED,                        FSSCAT_WARNING, "Placement field ignored.",                                         false,  FSCODE_NONE},
    {FSCODE_FILE_ON_PRINT_QUEUE,                            FSSCAT_WARNING, "File cannot be deleted because it is on the print queue.",         false,  FSCODE_NONE},
    {FSCODE_READ_KEY_EXISTS,                                FSSCAT_WARNING, "A read key exists on the file.",                                   false,  FSCODE_NONE},
    {FSCODE_CATALOGED_AS_READ_ONLY,                         FSSCAT_WARNING, "File is cataloged as a read-only file.",                           false,  FSCODE_NONE},
    {FSCODE_FILE_WAS_ASSIGNED_DURING_SYSTEM_FAILURE,        FSSCAT_WARNING, "File was assigned during system failure.",                         false,  FSCODE_NONE},
    {FSCODE_WRITE_KEY_EXISTS,                               FSSCAT_WARNING, "A write key exists on the file.",                                  false,  FSCODE_NONE},
    {FSCODE_CATALOGED_AS_WRITE_ONLY,                        FSSCAT_WARNING, "File is cataloged write-only.",                                    false,  FSCODE_NONE},
    {FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_IQXYZ,    FSSCAT_WARNING, "Option conflict with previous assign options, all options ignored except i, q, x, y, or z.",
                                                                                                                                                false,  FSCODE_NONE},
    {FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_MISC,     FSSCAT_WARNING, "Option conflict with previous assign options-option conflict ignored.",
                                                                                                                                                false,  FSCODE_NONE},
    {FSCODE_ALREADY_EXCLUSIVELY_ASSIGNED,                   FSSCAT_WARNING, "X option ignored, file already exclusively assigned by this run.", false,  FSCODE_NONE},
    {FSCODE_SYSTEM_CONFIGURED_TO_IGNORE_Z_OPTION_IN_BATCH,  FSSCAT_WARNING, "System configured to ignore Z option in batch.",                   false,  FSCODE_NONE},
    {FSCODE_SYSTEM_CONFIGURED_TO_RELEASE_INITIAL_RESERVE,   FSSCAT_WARNING, "System is configured to release unused mass storage space.",       false,  FSCODE_NONE},

    //  Errors
    {FSCODE_ASSIGN_MNEMONIC_IS_NOT_CONFIGURED,              FSSCAT_ERROR,   "% is not a configured assign mnemonic.",                           false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_OPTION_COMBINATION,                     FSSCAT_ERROR,   "Illegal option combination %%",                                    false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_OPTION,                                 FSSCAT_ERROR,   "Illegal option %",                                                 false,  FSCODE_NONE},
    {FSCODE_ASSIGN_MNEMONIC_DOES_NOT_SUPPORT_6_BIT_PACKED,  FSSCAT_ERROR,   "Assign mnemonic %s does not support 6-bit packed format.",         false,  FSCODE_NONE},
    {FSCODE_MNEMONIC_DOES_NOT_ALLOW_NOISE_FIELD,            FSSCAT_ERROR,   "Assign mnemonic %s does not allow a noise field.",                 false,  FSCODE_NONE},
    {FSCODE_ASSIGN_MNEMONIC_TOO_LONG,                       FSSCAT_ERROR,   "Assign mnemonic too long",                                         false,  FSCODE_NONE},
    {FSCODE_ASSIGN_MNEMONIC_MUST_BE_WORD_ADDRESSABLE,       FSSCAT_ERROR,   "Assign mnemonic must be word addressable",                         false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE,      FSSCAT_ERROR,   "Illegal attempt to change assignment type",                        false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_GENERIC_TYPE,                 FSSCAT_ERROR,   "Attempt to change generic type of the file.",                      false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_GRANULARITY,                  FSSCAT_ERROR,   "Attempt to change granularity.",                                   false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_INITIAL_RESERVE,              FSSCAT_ERROR,   "Attempt to change initial reserve of write inhibited file.",       false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_DIFFERENT_ACCOUNT,
                                                            FSSCAT_ERROR,   "Attempt to change maximum granules of a file cataloged under a different account.",
                                                                                                                                                false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_WRITE_INHIBITED, FSSCAT_ERROR,   "Attempt to change maximum granules on a write inhibited file.",    false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_FOR_FCYCLE,                       FSSCAT_ERROR,   "Illegal value specified for F-cycle.",                             false,  FSCODE_NONE},
    {FSCODE_FILE_CYCLE_OUT_OF_RANGE,                        FSSCAT_ERROR,   "File cycle out of range.",                                         false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_DROPPING_OF_PRIVATE_FILE,               FSSCAT_ERROR,   "Creation of file would require illegal dropping of private file.", false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_FORMAT,             FSSCAT_ERROR,   "Illegal value specified for data transfer format.",                false,  FSCODE_NONE},
    {FSCODE_CSI_SYNTAX_ERROR,                               FSSCAT_ERROR,   "Syntax error in image submitted to ER CSI$.",                      false,  FSCODE_NONE},
    {FSCODE_CSI_BAD_IMAGE_LENGTH,                           FSSCAT_ERROR,   "Length of image submitted to ER CSI$ is bad.",                     false,  FSCODE_NONE},
    {FSCODE_CSI_ILLEGAL_CONTROL_STATEMENT_TYPE,             FSSCAT_ERROR,   "Illegal control statement type submitted to ER CSI$.",             false,  FSCODE_NONE},
    {FSCODE_FILENAME_IS_REQUIRED,                           FSSCAT_ERROR,   "A filename is required on the image.",                             false,  FSCODE_NONE},
    {FSCODE_FILE_IS_ALREADY_CATALOGED,                      FSSCAT_ERROR,   "File is already catalogued.",                                      false,  FSCODE_NONE},
    {FSCODE_FILE_IS_NOT_CATALOGED,                          FSSCAT_ERROR,   "File is not catalogued.",                                          false,  FSCODE_NONE},
    {FSCODE_FILE_CANNOT_BE_RECOVERED_MFD_CORRUPTED,         FSSCAT_ERROR,   "File can not be recovered because master file directory (MFD) information has been corrupted.",
                                                                                                                                                false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_DELETE_VIA_FREE,                     FSSCAT_ERROR,   "Attempt to delete via @FREE,D but file was not assigned.",         false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_GRANULARITY,        FSSCAT_ERROR,   "Illegal value specified for granularity.",                         false,  FSCODE_NONE},
    {FSCODE_INTERNAL_NAME_IS_REQUIRED,                      FSSCAT_ERROR,   "Internal name is required.",                                       false,  FSCODE_NONE},
    {FSCODE_INSUFFICIENT_NUMBER_OF_UNITS_AVAILABLE,         FSSCAT_ERROR,   "Insufficient number of units available.",                          false,  FSCODE_NONE},
    {FSCODE_IO_ERROR_ENCOUNTERED_ON_MFD,                    FSSCAT_ERROR,   "I/O error encountered on the master file directory.",              false,  FSCODE_NONE},
    {FSCODE_FILE_INITIAL_RESERVE_EXCEEDED,                  FSSCAT_ERROR,   "File initial reserve granule limits exceeded.",                    false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_FOR_INITIAL_RESERVE,              FSSCAT_ERROR,   "Illegal value specified for initial reserve.",                     false,  FSCODE_NONE},
    {FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED,                  FSSCAT_ERROR,   "Read and/or write keys are needed.",                               false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_LOGICAL_CHANNEL,    FSSCAT_ERROR,   "Illegal value specified for logical channel.",                     false,  FSCODE_NONE},
    {FSCODE_MAX_GRANULES_LESS_THAN_HIGHEST_ALLOCATED,       FSSCAT_ERROR,   "Maximum granules less than highest granule allocated.",            false,  FSCODE_NONE},
    {FSCODE_FILE_MAX_GRANULES_EXCEEDED,                     FSSCAT_ERROR,   "File maximum granule limits exceeded.",                            false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_FOR_MAXIMUM,                      FSSCAT_ERROR,   "Illegal value specified for maximum.",                             false,  FSCODE_NONE},
    {FSCODE_MAXIMUM_IS_LESS_THAN_RESERVE,                   FSSCAT_ERROR,   "Maximum is less than the initial reserve.",                        false,  FSCODE_NONE},
    {FSCODE_MASS_STORAGE_OVERFLOW,                          FSSCAT_ERROR,   "Mass storage has overflowed.",                                     false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_VALUE_FOR_NUMBER_OF_UNITS,              FSSCAT_ERROR,   "Illegal value for number of units.",                               false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_NOISE_CONSTANT_VALUE,                   FSSCAT_ERROR,   "Illegal noise constant value.",                                    false,  FSCODE_NONE},
    {FSCODE_ILLEGAL_USE_OF_I_OPTION,                        FSSCAT_ERROR,   "Illegal use of I option.",                                         false,  FSCODE_NONE},
    {FSCODE_I_OPTION_ONLY_ON_USE,                           FSSCAT_ERROR,   "I option is the only legal option on USE.",                        false,  FSCODE_NONE},
    {FSCODE_FCYCLE_PLUS_ONE_ILLEGAL_WITH_A,                 FSSCAT_ERROR,   "F-cycle of +1 is illegal with A option.",                          false,  FSCODE_NONE},
    {FSCODE_INCORRECT_PRIVACY_KEY,                          FSSCAT_ERROR,   "Incorrect privacy key for private file.",                          false,  FSCODE_NONE},
    {FSCODE_INCORRECT_READ_KEY,                             FSSCAT_ERROR,   "Incorrect read key.",                                              false,  FSCODE_NONE},
    {FSCODE_FILE_NOT_CATALOGED_WITH_READ_KEY,               FSSCAT_ERROR,   "File is not cataloged with read key.",                             false,  FSCODE_NONE},
    {FSCODE_RELATIVE_FCYCLE_CONFLICT,                       FSSCAT_ERROR,   "Relative F-cycle conflict.",                                       false,  FSCODE_NONE},
    {FSCODE_FILE_BACKUP_IS_NOT_AVAILABLE,                   FSSCAT_ERROR,   "File backup is not available.",                                    false,  FSCODE_NONE},
    {FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD,     FSSCAT_ERROR,   "Image contains an undefined field or subfield.",                   false,  FSCODE_NONE},
    {FSCODE_UNITS_AND_LOGICAL_SUBFIELDS_NOT_ALLOWED_FOR_CAT,FSSCAT_ERROR,   "Units and logical subfields not allowed on CAT image.",            false,  FSCODE_NONE},
    {FSCODE_ATTEMPT_TO_CHANGE_TO_WORD_ADDRESSABLE_NOT_ALLOWED,
                                                            FSSCAT_ERROR,   "Attempt to change to word addressable not allowed.",               false,  FSCODE_NONE},
    {FSCODE_INCORRECT_WRITE_KEY,                            FSSCAT_ERROR,   "Incorrect write key.",                                             false,  FSCODE_NONE},
    {FSCODE_FILE_NOT_CATALOGED_WITH_WRITE_KEY,              FSSCAT_ERROR,   "File is not cataloged with write key.",                            false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_REEL_REJECTED_Z,                       FSSCAT_ERROR,   "Hold for reel rejected because of Z option.",                      false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_PACK_REJECTED_Z,                       FSSCAT_ERROR,   "Hold for pack rejected because of Z option.",                      false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_TAPE_UNIT_REJECTED_Z,                  FSSCAT_ERROR,   "Hold for tape unit rejected because of Z option.",                 false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_DISK_UNIT_REJECTED_Z,                  FSSCAT_ERROR,   "Hold for disk unit rejected because of Z option.",                 false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_X_USE_REJECTED_Z,                      FSSCAT_ERROR,   "Hold for x-use rejected because of Z option.",                     false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_RELEASE_OF_X_USE_REJECTED_Z,           FSSCAT_ERROR,   "Hold for release of x--use rejected because of Z option.",         false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_ROLLBACK_REJECTED_Z,                   FSSCAT_ERROR,   "Hold for rollback of unloaded file rejected because of Z option.", false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_COM_LINE_REJECTED_Z,                   FSSCAT_ERROR,   "Hold for com line rejected because of Z option.",                  false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_COM_GROUP_REJECTED_Z,                  FSSCAT_ERROR,   "Hold for com group rejected because of Z option.",                 false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_REM_DISK_REJECTED_Z,                   FSSCAT_ERROR,   "Hold for *rem disk rejected because of Z option.",                 false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_DEV_CU_REJECTED_Z,                     FSSCAT_ERROR,   "Hold for *DEV/CU rejected because of Z option.",                   false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_FCYCLE_CONFLICT_REJECTED_Z,            FSSCAT_ERROR,   "Hold for F-cycle conflict rejected because of Z option.",          false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_MASS_STORAGE_SPACE_REJECTED_Z,         FSSCAT_ERROR,   "Hold for mass storage space rejected because of Z option.",        false,  FSCODE_NONE},
    {FSCODE_RUN_ABORTED,                                    FSSCAT_ERROR,   "Run has been aborted.",                                            false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_CONTROL_OF_CACHING_REJECTED_Z,         FSSCAT_ERROR,   "Hold for control of caching rejected because of Z option.",        false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_DISKETTE_UNIT_REJECTED_Z,              FSSCAT_ERROR,   "Hold for diskette unit rejected because of Z option.",             false,  FSCODE_NONE},
    {FSCODE_DIRECTORY_ID_AND_QUALIFIER_NOT_ALLOWED,         FSSCAT_ERROR,   "Directory id and qualifier may not appear on image when R option is used.",
                                                                                                                                            false,  FSCODE_NONE},
    {FSCODE_DIRECTORY_ID_OR_QUALIFIER_REQUIRED,             FSSCAT_ERROR,   "Directory id or qualifier must appear on image.",                  false,  FSCODE_NONE},
    {FSCODE_NO_TAPE_UNITS_CONFIGURED,                       FSSCAT_ERROR,   "No tape units are configured on this system. Tape unit cannot be allocated.",
                                                                                                                                            false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_CTL_REJECTED_Z,                        FSSCAT_ERROR,   "Hold for CTL availability rejected due to `Z' option.",            false,  FSCODE_NONE},
    {FSCODE_HOLD_FOR_NETWORK_DEVICE_REJECTED_Z,             FSSCAT_ERROR,   "Hold for network device availability because of the Z option.",    false,  FSCODE_NONE},
};

COUNT FacilitiesManager::m_StatusCodeInfoTableSize = sizeof( FacilitiesManager::m_StatusCodeInfoTable ) / sizeof (FacilitiesManager::StatusCodeInfo);



//  private methods

//  checkCycleConstraints()
//
//  Check cycle constraints for the case where no file set exists.
//  We accept absolute cycles from 1 to 999, and +1 for relative cycles.
//  We do *not* accept relative cycles 0 or -1 through -31, since that makes no sense in a
//  @CAT context.  It is not an error if no cycle is specified.
//
//  Parameters:
//      context:            context under which we operate
//      effectiveSpec:      file specification containing file cycle
//
//  Returns:
//      true if cycle is not specified, or if it is specified and valid.
bool
FacilitiesManager::checkCycleConstraints
(
    const Context&              context,
    const FileSpecification&    effectiveSpec
) const
{
    if ( effectiveSpec.m_AbsoluteCycleSpecified )
    {
        if (( effectiveSpec.m_Cycle < 1 ) || ( effectiveSpec.m_Cycle > 999 ))
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_FOR_FCYCLE );
            context.m_pResult->m_StatusBitMask.setW( 0600000000040 );
            return false;
        }
    }

    if (( effectiveSpec.m_RelativeCycleSpecified ) && ( effectiveSpec.m_Cycle != 1 ))
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_FOR_FCYCLE );
        context.m_pResult->m_StatusBitMask.setW( 0600000000040 );
        return false;
    }

    return true;
}


//  checkCycleConstraints()
//
//  Assuming a file set already exists for the requested @CAT or @ASG,C/P, we check cycle constraints.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      pRunInfo:           pointer to RunInfo object of interest
//      effectiveFileSpec:  effective file specification (after walking through @USE items)
//      fileSetInfo:        FileSetInfo object retrieved from MFD manager for the existing file set
//      pResult:            where we store result
//      pAbsoluteCycle:     where we store the requested or determined absolute cycle
bool
FacilitiesManager::checkCycleConstraints
(
    const Context&                  context,
    const FileSpecification&        effectiveFileSpec,
    const MFDManager::FileSetInfo&  fileSetInfo,
    UINT16* const                   pAbsoluteCycle
) const
{
    //  Does the requested file cycle already exist?
    if ( !checkForExistingCycle( context, effectiveFileSpec, fileSetInfo, false ) )
        return false;

    //  Does a +1 cycle exist?
    if ( fileSetInfo.m_PlusOneCycleExists )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_CYCLE_OUT_OF_RANGE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0400000000040ll );
        return false;
    }

    //  Would creating this new absolute cycle blow-out the range (bearing in mind, we cannot delete more than one cycle)?
    //  To find out, let's first figure out the absolute cycle (if not already specified)...
    *pAbsoluteCycle = effectiveFileSpec.m_Cycle;
    if ( !effectiveFileSpec.m_AbsoluteCycleSpecified )
    {
        //  +1 is the only relative cycle allowed, if the fileset already exists (which it does, if we're here).
        if ( (effectiveFileSpec.m_Cycle != 1) || fileSetInfo.m_PlusOneCycleExists )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_CYCLE_OUT_OF_RANGE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400000000040 );
            return false;
        }

        //  If the top-level cycle(s) has/have been deleted, use the top-level space to determine the absolute cycle.
        //  Otherwise, use the top-level space plus one.
        if ( !fileSetInfo.m_CycleEntries[0].m_Exists )
            *pAbsoluteCycle = fileSetInfo.m_CycleEntries[0].m_AbsoluteCycle;
        else
            *pAbsoluteCycle = fileSetInfo.m_CycleEntries[0].m_AbsoluteCycle + 1;
    }

    //  Find the relative position of the requested absolute cycle, compared to the existing highest absolute cycle.
    //  This helps us decide whether we are expanding the file cycle range, and whether above or below the existing set.
    int reqPosition = *pAbsoluteCycle - fileSetInfo.m_CycleEntries[0].m_AbsoluteCycle;
    if ( reqPosition > 31 )
        reqPosition -= 999;
    else if ( reqPosition < -31 )
        reqPosition += 999;

    //  Are we extending the cycle range above the existing highest cycle?
    if ( reqPosition > 0 )
    {
        //  Yes - find out how many existing cycles are now outside the max range
        COUNT outsideCount = 0;
        INDEX lax = fileSetInfo.m_MaxRange - reqPosition - 1;   // lowest index at which we allow an existing cycle in the vector of cycle infos
        INDEX lx = 0;                                           // index of last existing cycle outside of the max range (if any)
        for ( INDEX cx = lax; cx < fileSetInfo.m_CycleEntries.size(); ++cx )
        {
            if ( fileSetInfo.m_CycleEntries[cx].m_Exists )
            {
                ++outsideCount;
                lx = cx;
            }
        }

        if ( outsideCount > 1 )
        {
            //  Operation not allowed - cannot drop more than one cycle automatically
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_CYCLE_OUT_OF_RANGE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400000000040ll );
            return false;
        }

        else if ( outsideCount == 1 )
        {
            //  We're going to need to drop a cycle - is this doable at this time?
            MFDManager::FileCycleInfo fcInfo;
            MFDManager::Result mfdResult =
                m_pMFDManager->getFileCycleInfo( fileSetInfo.m_CycleEntries[lx].m_MainItem0Addr, &fcInfo );
            if ( !checkMFDResult( mfdResult, context.m_pResult ) )
            {
                logMFDError( context, "FacilitiesManager::checkCycleConstraints", effectiveFileSpec, mfdResult );
                return false;
            }

            //  Is the cycle being dropped, accessible sufficiently to be dropped?
            //  - Private-by-account requires account match with RunInfo
            if ( m_PrivateByAccount
                && ( fcInfo.m_AccountId.compareNoCase( context.m_pRunInfo->getAccountId() ) == 0 )
                && !context.m_pSecurityContext->canBypassPrivacyInhibit() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_DROPPING_OF_PRIVATE_FILE );
                context.m_pResult->m_StatusBitMask.logicalOr( 0400000020000ll );
                return false;
            }

            //  If the cycle to be dropped is assigned or on an output queue, we can't do it.
            if ( (fcInfo.m_CurrentAssignCount > 0) || fcInfo.m_Queued )
            {
                //  Operation not allowed - cannot drop the oldest cycle because it is currently assigned.
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_HELD_FOR_FILE_CYCLE_CONFLICT );
                context.m_pResult->m_StatusBitMask.logicalOr( 0400001000040ll );
                return false;
            }
        }
    }
    else
    {
        //  reqPosition must be < 0, which is okay, but we can't go so low that we blow out the range.
        if ( -reqPosition >= static_cast<int>(fileSetInfo.m_MaxRange) )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_CYCLE_OUT_OF_RANGE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400000000040ll );
            return false;
        }
    }

    return true;
}


//  checkDiskEquipmentType()
//
//  Checks a given assign mnemonic to make sure it is valid and does any appropriate compatibility checks.
//  The results are stored in the given DiskSubfieldInfo object, and any errors are posted into pResult.
//  It is not an error if the subfield is not specified.
//
//  Don't call here if all of the following are true:
//      () the equipment type was specified (or inferred),
//      () no FacilityItem exists (file is not already assigned),
//      () no FileSetInfo / FileCycleInfo exists (file is not cataloged)
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:   from @ASG or @CAT processing
//      pFileCycleInfo:     If a relevant file is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:           If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:              where we store a pointer to the EquipmentType object if found
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskEquipmentType
(
    const Context&                                      context,
    const FieldList&                                    additionalFields,
    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
    const FacilityItem* const                           pFacItem,
    DiskSubfieldInfo* const                             pInfo
) const
{
    if ( (additionalFields.size() > EQUIPMENT_TYPE_FIELD)
        && (additionalFields[EQUIPMENT_TYPE_FIELD].size() > EQUIPMENT_TYPE_SUBFIELD)
        && (additionalFields[EQUIPMENT_TYPE_FIELD][EQUIPMENT_TYPE_SUBFIELD].size() > 0) )
    {
        SuperString mnemonic = additionalFields[EQUIPMENT_TYPE_FIELD][EQUIPMENT_TYPE_SUBFIELD];
        mnemonic.foldToUpperCase();

        //  Make sure the given type is valid
        if ( mnemonic.size() > 6 )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ASSIGN_MNEMONIC_TOO_LONG );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return false;
        }

        //  Make sure the given type is configured
        pInfo->m_pEquipmentType = findEquipmentType( mnemonic );
        if ( !pInfo->m_pEquipmentType )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ASSIGN_MNEMONIC_IS_NOT_CONFIGURED, mnemonic ) );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000 );
            return false;
        }

        //  We know that the equipment type should be disk - verify that.
        if ( pInfo->m_pEquipmentType->m_EquipmentCategory != ECAT_DISK )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_GENERIC_TYPE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
            return false;
        }

        //  If a FacilityItem was provided, then the file is already assigned and the requested equipment type
        //  must be generally compatible with the Facilities Item... otherwise, if a FileCycleInfo was provided,
        //  then the file is already cataloged and the requested type must be generally compatible with that.
        if ( pFacItem )
        {
            //  File is not cataloged, but this is @ASG,C/U/T already assigned to the run.
            //  Look for word/sector addressable conflict in equiptype, compared to the fac item.
            if ( (pInfo->m_pEquipmentType->m_EquipmentTypeGroup == FACETG_WORD_DISK) && pFacItem->isSectorMassStorage() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_TO_WORD_ADDRESSABLE_NOT_ALLOWED );
                context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
                return false;
            }
            else if ( (pInfo->m_pEquipmentType->m_EquipmentTypeGroup == FACETG_SECTOR_DISK) && pFacItem->isWordMassStorage() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ASSIGN_MNEMONIC_MUST_BE_WORD_ADDRESSABLE );
                context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
                return false;
            }
        }
        else if ( pFileCycleInfo )
        {
            //  The file is already cataloged - look for word/sector-addressable conflicts
            if ( (pInfo->m_pEquipmentType->m_EquipmentTypeGroup == FACETG_WORD_DISK) && !pFileCycleInfo->m_WordAddressable )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_TO_WORD_ADDRESSABLE_NOT_ALLOWED );
                context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
                return false;
            }

            if ( (pInfo->m_pEquipmentType->m_EquipmentTypeGroup == FACETG_SECTOR_DISK) && pFileCycleInfo->m_WordAddressable )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ASSIGN_MNEMONIC_MUST_BE_WORD_ADDRESSABLE );
                context.m_pResult->m_StatusBitMask.logicalOr( 0420000000000LL );
                return false;
            }
        }
    }

    return true;
}


//  checkDiskInitialReserve()
//
//  Validates the initial reserve field (if it exists).
//  The results are stored in the given DiskSubfieldInfo object, and any errors are posted into pResult.
//  It is not an error if the subfield was not specified.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:       from @ASG or @CAT processing
//      pFileCycleInfo:         If a relevant file is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:               If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:                  where we store initial reserve info if specified, along with the flag indicating whether it was specified
//                                  - caller must have set m_pEquipmentType before invoking this routine
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskInitialReserve
(
    const Context&                                      context,
    const FieldList&                                    additionalFields,
    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
    const FacilityItem* const                           pFacItem,
    DiskSubfieldInfo* const                             pInfo
) const
{
    pInfo->m_InitialReserveSpecified = false;

    if ( (additionalFields.size() > DISK_INITIAL_RESERVE_FIELD)
        && (additionalFields[DISK_INITIAL_RESERVE_FIELD].size() > DISK_INITIAL_RESERVE_SUBFIELD)
        && (additionalFields[DISK_INITIAL_RESERVE_FIELD][DISK_INITIAL_RESERVE_SUBFIELD].size() > 0) )
    {
        SuperString reserveStr = additionalFields[DISK_INITIAL_RESERVE_FIELD][DISK_INITIAL_RESERVE_SUBFIELD];
        if ( !reserveStr.isDecimalNumeric() )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_FOR_INITIAL_RESERVE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return false;
        }

        pInfo->m_InitialReserve = reserveStr.toDecimal();
        pInfo->m_InitialReserveSpecified = true;

        if ( pInfo->m_InitialReserve > 0777777 )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_INITIAL_RESERVE_EXCEEDED );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
            return false;
        }

        //  Attempt to change already-cataloged (or to-be-cataloged) file?
        if ( !context.m_pSecurityContext->canBypassReadWriteInhibits() && (pFileCycleInfo != 0) && pFileCycleInfo->m_WriteInhibited )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_INITIAL_RESERVE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
            return false;
        }

        //  Are we configured to release unused initial reserve?
        if ( m_ReleaseUnusedReserve )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_SYSTEM_CONFIGURED_TO_RELEASE_INITIAL_RESERVE );
        }
    }

    return true;
}


//  checkDiskSubfields()
//
//  Parses the additional subfields for a disk @ASG or @CAT image (beyond the eqiupment type).
//  The results are stored in the given DiskSubfieldInfo object.
//  Any errors are posted into pResult.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:   from @ASG or @CAT processing
//      pFileCycleInfo:     If a relevant file cycle is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:           If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:              where we store values we derived from the additional fields...
//      catalogRequest:     true if this was invoked via @CAT
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskSubfields
(
    const Context&                                      context,
    const FieldList&                                    additionalFields,
    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
    const FacilityItem* const                           pFacItem,
    DiskSubfieldInfo* const                             pInfo,
    const bool                                          catalogRequest
) const
{
    //  additionalFields[0] will contain some combination of:
    //      equip-type,initial-reserve,granularity,max-granules,placement,...

    //  If the m_pEquipmentType field is non-zero, then it has been preset by the caller
    //  (or one of its ancestors) -- this always indicates that the file is not already assigned,
    //  and that it is not already cataloged, hence no compatibility checks are needed.
    //  Thus, we only do the full equip type check if that value is 0.
    if ( !pInfo->m_pEquipmentType )
    {
        if ( !checkDiskEquipmentType( context, additionalFields, pFileCycleInfo, pFacItem, pInfo ) )
            return false;
    }

    if ( !checkDiskInitialReserve( context, additionalFields, pFileCycleInfo, pFacItem, pInfo ) )
        return false;
    if ( !checkDiskGranularity( context, additionalFields, pFileCycleInfo, pFacItem, pInfo ) )
        return false;
    if ( !checkDiskMaxGranules( context, additionalFields, pFileCycleInfo, pFacItem, pInfo ) )
        return false;
    if ( !checkDiskPlacement( context, additionalFields, catalogRequest ) )
        return false;

    if ( ((additionalFields.size() > 0) && (additionalFields[0].size() > 5))
        || (additionalFields.size() > 1) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }

    return true;
}


//  checkEquipmentType()
//
//  Checks a given assign mnemonic to make sure it is valid and configured.
//  Any errors are posted into pResult.
//  This routine is invoked by callers who need to know whether to take a disk or tape path,
//  and by check*Subfields() when that routine's caller did not specify an EquipmentType pointer.
//
//  It is not an error if the subfield is not specified, but the caller may consider it so.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:       from @ASG or @CAT processing
//      pFileCycleInfo:         If a relevant file is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:               If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:                  where we store a pointer to the EquipmentType object if found
//
//  Returns:
//      pointer to EquipmentType object if found, else zero.
const FacilitiesManager::EquipmentType*
FacilitiesManager::checkEquipmentType
(
    const Context&          context,
    const FieldList&        additionalFields
) const
{
    const EquipmentType* pType = 0;

    if ( (additionalFields.size() > EQUIPMENT_TYPE_FIELD)
        && (additionalFields[EQUIPMENT_TYPE_FIELD].size() > EQUIPMENT_TYPE_SUBFIELD)
        && (additionalFields[EQUIPMENT_TYPE_FIELD][EQUIPMENT_TYPE_SUBFIELD].size() > 0) )
    {
        SuperString mnemonic = additionalFields[EQUIPMENT_TYPE_FIELD][EQUIPMENT_TYPE_SUBFIELD];
        mnemonic.foldToUpperCase();

        //  Make sure the given type is valid
        if ( mnemonic.size() > 6 )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ASSIGN_MNEMONIC_TOO_LONG );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return 0;
        }

        //  Make sure the given type is configured
        pType = findEquipmentType( mnemonic );
        if ( !pType )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ASSIGN_MNEMONIC_IS_NOT_CONFIGURED, mnemonic ) );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000 );
            return 0;
        }
    }

    return pType;
}


//  checkPrivate()
//
//  For @ASG, checks file privacy to ensure the caller can assign the file.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      pFileSetInfo:       pointer to FileSetInfo from MFDManager
//      pFileCycleInfo:     pointer to base class FileCycleInfo from MFDManager
//
//  Returns:
//      true if all given options are in the list of allowed options, else false
bool
FacilitiesManager::checkPrivate
(
    const Context&                          context,
    const MFDManager::FileSetInfo* const    pFileSetInfo,
    const MFDManager::FileCycleInfo* const  pFileCycleInfo
) const
{
    if ( pFileCycleInfo->m_Private && !context.m_pSecurityContext->canBypassPrivacyInhibit() )
    {
        bool allow = true;
        if ( m_PrivateByAccount )
        {
            if ( context.m_pRunInfo->getAccountId().compare( pFileCycleInfo->m_AccountId ) != 0 )
                allow = false;
        }
        else
        {
            if ( context.m_pRunInfo->getProjectId().compare( pFileSetInfo->m_ProjectId ) != 0 )
                allow = false;
        }

        if ( !allow )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_INCORRECT_PRIVACY_KEY );
            context.m_pResult->m_StatusBitMask.setW( 0400000020000 );
            return false;
        }
    }

    return true;
}


//  checkTapeSubfields()
//
//  Parses the additional subfields for a tape @ASG or @CAT image.
//  The results are stored in the given TapeSubfieldInfo object.
//  Any errors are posted into pResult.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:   from @ASG or @CAT processing
//      pFileSetInfo:       If a relevant file set is already cataloged, this is the FileSetInfo object from MFDManager (else 0)
//      pFileCycleInfo:     If a relevant file cycle is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:           If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:              where we store values we derived from the additional fields...
//      pResult:            Result object into which we may inject some status codes/bits
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkTapeSubfields
(
    const Context&                              context,
    const FieldList&                            additionalFields,
    const MFDManager::FileSetInfo* const        pFileSetInfo,
    const MFDManager::TapeFileCycleInfo* const  pFileCycleInfo,
    const FacilityItem* const                   pFacItem,
    TapeSubfieldInfo* const                     pInfo
) const
{
#if 0 //TODO:TAPE convert all the following for tape
    //  additionalFields[0] will contain some combination of:
    //      equip-type,initial-reserve,granularity,max-granules,placement,...

    //  If the m_pEquipmentType field is non-zero, then it has been preset by the caller
    //  (or one of its ancestors) -- this always indicates that the file is not already assigned,
    //  and that it is not already cataloged, hence no compatibility checks are needed.
    //  Thus, we only do the full equip type check if that value is 0.
    if ( !pInfo->m_pEquipmentType )
    {
        if ( !checkDiskEquipmentType( additionalFields, pFileCycleInfo, pFacItem, pInfo, pResult ) )
            return false;
    }

    if ( !checkDiskInitialReserve( pRunInfo, additionalFields, pFileCycleInfo, pFacItem, pInfo, pResult ) )
        return false;
    if ( !checkDiskGranularity( additionalFields, pFileCycleInfo, pFacItem, pInfo, pResult ) )
        return false;
    if ( !checkDiskMaxGranules( pRunInfo, additionalFields, pFileCycleInfo, pFacItem, pInfo, pResult ) )
        return false;
    if ( !checkDiskPlacement( additionalFields, pResult ) )
        return false;

    if ( ((additionalFields.size() > 0) && (additionalFields[0].size() > 5))
        || (additionalFields.size() > 1) )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD );
        context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }
#endif

    return true;
}


//  checkZOption()
//
//  For some commands, Z option may or may not be allowed for batch jobs in control mode.
//  We do the checking for that here.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      options:            command options
//
//  Returns:
//      true if processing should continue (i.e, we detect no errors),
//      false if processing should stop (transction should be committed, and result returned to caller)
bool
FacilitiesManager::checkZOption
(
    const Context&          context,
    const UINT32            options
) const
{
    if ( context.m_pRunInfo->isBatch() && context.m_pRunInfo->inControlMode() && ((options & OPTB_Z) != 0) )
    {
        if ( m_RejectZOptionOnBatch )
        {
            UserRunInfo* prui = dynamic_cast<UserRunInfo*>( context.m_pRunInfo );
            prui->setErrorMode();
            context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ILLEGAL_OPTION, "Z" ) );
            return false;
        }
        else
        {
            //  Post warning to result
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_SYSTEM_CONFIGURED_TO_IGNORE_Z_OPTION_IN_BATCH );
        }
    }

    return true;
}


//  findEquipmentType
//
//  Find EquipType struct matching the given mnemonic
const FacilitiesManager::EquipmentType*
FacilitiesManager::findEquipmentType
(
    const std::string&      mnemonic
) const
{
    SuperString temp = mnemonic;
    temp.foldToUpperCase();
    for ( INDEX ex = 0; ex < m_EquipmentTypes.size(); ++ex )
    {
        if ( temp.compare( m_EquipmentTypes[ex]->m_pMnemonic ) == 0 )
            return m_EquipmentTypes[ex];
    }

    return 0;
}


//  findLeadAndMainItemInfoDisk()
//
//  Retrieves FileSetInfo and FileCycleInfo information for a cataloged file cycle.
//
//  Parameters:
//      fileSpecification:      describes the file set and cycle of interest.  This must be an *effective* file spec.
//      pFileSetInfo:           pointer to object to be populated
//      pCycleEntryIndex:       where we store a pointer to the index of the relevant CycleEntry object in the FileSetInfo object
//      pFileCycleInfo:         pointer to object to be populated
//
//  Returns:
//      true if we found the file set and the file cycle, else false
bool
FacilitiesManager::findLeadAndMainItemInfoDisk
(
    const FileSpecification&                    fileSpecification,
    MFDManager::FileSetInfo* const              pFileSetInfo,
    INDEX* const                                pCycleEntryIndex,
    MFDManager::MassStorageFileCycleInfo* const pFileCycleInfo
) const
{
    MFDManager::Result mfdResult = m_pMFDManager->getFileSetInfo( fileSpecification.m_Qualifier,
                                                                  fileSpecification.m_FileName,
                                                                  pFileSetInfo );
    Result result;
    if ( !checkMFDResult( mfdResult, &result ) )
    {
        logMFDError( "FacilitiesManager::findLeadAndMainItemInfoDisk(a)", fileSpecification, mfdResult );
        return false;
    }

    if ( !findExistingCycleEntryIndex( fileSpecification, *pFileSetInfo, pCycleEntryIndex ) )
        return false;

    mfdResult = m_pMFDManager->getMassStorageFileCycleInfo( pFileSetInfo->m_CycleEntries[*pCycleEntryIndex].m_MainItem0Addr, pFileCycleInfo );
    if ( !checkMFDResult( mfdResult, &result ) )
    {
        logMFDError( "FacilitiesManager::findLeadAndMainItemInfoDisk(b)", fileSpecification, mfdResult );
        return false;
    }

    return true;
}


//  findLeadItemInfo()
//
//  Retrieves FileSetInfo information for a cataloged file cycle.
//
//  Parameters:
//      fileSpecification:      describes the file set and cycle of interest.  This must be an *effective* file spec.
//      pFileSetInfo:           pointer to object to be populated
//      pCycleEntryIndex:       where we store a pointer to the index of the relevant CycleEntry object in the FileSetInfo object
//
//  Returns:
//      true if we found the file set and the file cycle, else false
bool
FacilitiesManager::findLeadItemInfo
(
    const FileSpecification&        fileSpecification,
    MFDManager::FileSetInfo* const  pFileSetInfo,
    INDEX* const                    pCycleEntryIndex
) const
{
    Result result;
    MFDManager::Result mfdResult = m_pMFDManager->getFileSetInfo( fileSpecification.m_Qualifier,
                                                                  fileSpecification.m_FileName,
                                                                  pFileSetInfo );
    if ( !checkMFDResult( mfdResult, &result ) )
    {
        logMFDError( "FacilitiesManager::findLeadItemInfo", fileSpecification, mfdResult );
        return false;
    }

    if ( !findExistingCycleEntryIndex( fileSpecification, *pFileSetInfo, pCycleEntryIndex ) )
        return false;

    return true;
}



//  private static methods

//  checkDisables()
//
//  Check various disable flags to see whether we're allowed to assign the file.  For @ASG.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      options:            assign options
//      diskFlag:           true for disk files, false for tape
//      fileCycleInfo:      FileCycleInfo containing disable flags from the MFD
//      pReadDisabled:      we set this true if fac item read-disabled flag should be set
//      pWriteDisabled:     we set this true if fac item write-disabled flag should be set
//
//  Returns:
//      true if processing should continue (i.e, we detect no errors),
//      false if processing should stop
bool
FacilitiesManager::checkDisables
(
    const Context&                      context,
    const UINT32                        options,
    const bool                          diskFlag,
    const MFDManager::FileCycleInfo&    fileCycleInfo,
    bool* const                         pReadDisabled,
    bool* const                         pWriteDisabled
)
{
    bool EOpt = diskFlag ? (options & OPTB_E) > 0 : false;
    bool QOpt = (options & OPTB_Q) > 0;
    bool YOpt = (options & OPTB_Y) > 0;

    //  Always warn on simple assigned-during-system-crash disable
    if ( fileCycleInfo.m_WrittenToDisabled )
    {
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_WAS_ASSIGNED_DURING_SYSTEM_FAILURE );
        context.m_pResult->m_StatusBitMask.logicalOr( 0000000000200LL );
    }

    //  If this is a disk file and E option is given, or if Y option is given, ignore al file disable flags
    //  and set read and write disables.
    if ( EOpt || YOpt )
    {
        *pReadDisabled = true;
        *pWriteDisabled = true;
    }

    //  Else if Q Opt is given, ignore all file disable flags, and do not set read and write disabled
    else if ( QOpt )
    {
        //  Nothing to do here
    }

    //  For all other cases, honor file disable flags
    else
    {
        //  Is there a known problem with MFD items for this file?
        if ( fileCycleInfo.m_DirectoryDisabled )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_WAS_ASSIGNED_DURING_SYSTEM_FAILURE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400000000400LL );
            return false;
        }

        //  Was it rolled out, with the backup copy unrecoverable?
        if ( fileCycleInfo.m_InaccessibleBackupDisabled )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_BACKUP_IS_NOT_AVAILABLE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400000000100LL );
            return false;
        }
    }

    //  If caller is not privileged, check cataloged file read-disabled/write-disabled status...
    if ( !context.m_pSecurityContext->canBypassReadWriteInhibits() )
    {
        if ( fileCycleInfo.m_ReadInhibited )
            *pReadDisabled = true;
        if ( fileCycleInfo.m_WriteInhibited )
            *pWriteDisabled = true;
    }

    return true;
}


//  checkDiskGranularity()
//
//  Checks the granularity subfield to make sure it is valid, and does any appropriate compatibility checks.
//  The results are stored in the given DiskSubfieldInfo object, and any errors are posted into pResult.
//  It is not an error if the subfield is not specified.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:   from @ASG or @CAT processing
//      pFileCycleInfo:     If a relevant file is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:           If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:              where we store a pointer to the EquipmentType object if found
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskGranularity
(
    const Context&                                      context,
    const FieldList&                                    additionalFields,
    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
    const FacilityItem* const                           pFacItem,
    DiskSubfieldInfo* const                             pInfo
)
{
    pInfo->m_GranularitySpecified = false;

    if ( (additionalFields.size() > DISK_GRANULARITY_FIELD)
        && (additionalFields[DISK_GRANULARITY_FIELD].size() > DISK_GRANULARITY_SUBFIELD)
        && (additionalFields[DISK_GRANULARITY_FIELD][DISK_GRANULARITY_SUBFIELD].size() > 0) )
    {
        SuperString granuleStr = additionalFields[DISK_GRANULARITY_FIELD][DISK_GRANULARITY_SUBFIELD];
        if ( granuleStr.compareNoCase( "TRK" ) == 0 )
            pInfo->m_Granularity = MSGRAN_TRACK;
        else if ( granuleStr.compareNoCase( "POS" ) == 0 )
            pInfo->m_Granularity = MSGRAN_POSITION;
        else
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_GRANULARITY );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return false;
        }

        //  Look for conflicts...
        if ( pFacItem )
        {
            bool attemptToChange = false;
            const DiskFacilityItem* pDiskFacItem = dynamic_cast<const DiskFacilityItem*>( pFacItem );
            if ( pInfo->m_Granularity != pDiskFacItem->getGranularity() )
            {
                attemptToChange = true;
            }
            else if ( pFileCycleInfo && ((pInfo->m_Granularity == MSGRAN_POSITION) != pFileCycleInfo->m_PositionGranularity) )
            {
                attemptToChange = true;
            }

            if ( attemptToChange )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_GRANULARITY );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }
        }

        pInfo->m_GranularitySpecified = true;
    }

    return true;
}


//  checkDiskMaxGranules()
//
//  Validates the max granules subfield (if it exists).
//  The results are stored in the given DiskSubfieldInfo object, and any errors are posted into pResult.
//  It is not an error if the subfield was not specified.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      additionalFields:   from @ASG or @CAT processing
//      pFileCycleInfo:     If a relevant file is already cataloged, this is the FileCycleInfo object from MFDManager (else 0)
//      pFacItem:           If the file is already assigned, this is the pointer to the relevant FacilityItem object (else 0)
//      pInfo:              where we store initial reserve info if specified, along with the flag indicating whether it was specified
//                              - caller must have set m_pEquipmentType before invoking this routine
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskMaxGranules
(
    const Context&                                      context,
    const FieldList&                                    additionalFields,
    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
    const FacilityItem* const                           pFacItem,
    DiskSubfieldInfo* const                             pInfo
)
{
    pInfo->m_MaxGranulesSpecified = false;

    if ( (additionalFields.size() > DISK_MAX_GRANULES_FIELD)
        && (additionalFields[DISK_MAX_GRANULES_FIELD].size() > DISK_MAX_GRANULES_SUBFIELD)
        && (additionalFields[DISK_MAX_GRANULES_FIELD][DISK_MAX_GRANULES_SUBFIELD].size() > 0) )
    {
        SuperString maxGranulesStr = additionalFields[DISK_MAX_GRANULES_FIELD][DISK_MAX_GRANULES_SUBFIELD];
        if ( !maxGranulesStr.isDecimalNumeric() )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_VALUE_FOR_MAXIMUM );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
            return false;
        }

        pInfo->m_MaxGranules = maxGranulesStr.toDecimal();
        if ( pInfo->m_MaxGranules > 0777777 )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_MAX_GRANULES_EXCEEDED );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
            return false;
        }

        //  If this is a pre-cataloged file, we have some additional checks
        if ( pFileCycleInfo )
        {
            if ( (context.m_pRunInfo->getAccountId().compare( pFileCycleInfo->m_AccountId ) != 0)
                && !context.m_pSecurityContext->canBypassAccountCheck() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_DIFFERENT_ACCOUNT );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }

            if ( pFileCycleInfo->m_WriteInhibited && !context.m_pSecurityContext->canBypassReadWriteInhibits() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_WRITE_INHIBITED );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }
        }

        //  If initial reserve was updated in the facilities request, make sure max is >= init
        if ( pInfo->m_InitialReserveSpecified && ( pInfo->m_MaxGranules < pInfo->m_InitialReserve ) )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MAXIMUM_IS_LESS_THAN_RESERVE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
            return false;
        }

        //  If there is an MFD entry, verify against the initial and allocation fields.
        if ( pFileCycleInfo )
        {
            if ( pInfo->m_MaxGranules < pFileCycleInfo->m_InitialReserve )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MAXIMUM_IS_LESS_THAN_RESERVE );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }

            if ( pInfo->m_MaxGranules < pFileCycleInfo->m_HighestGranuleAssigned )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MAX_GRANULES_LESS_THAN_HIGHEST_ALLOCATED );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }
        }

        //  In the absence of MFD information, *if* there is a Facilities item,
        //  verify against what we know in the FacItem.
        else if ( pFacItem )
        {
            const DiskFacilityItem* pDiskFacItem = dynamic_cast<const DiskFacilityItem*>( pFacItem );

            if ( pInfo->m_MaxGranules < pDiskFacItem->getInitialGranules() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MAXIMUM_IS_LESS_THAN_RESERVE );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }

            if ( pInfo->m_MaxGranules < pDiskFacItem->getHighestGranuleAssigned() )
            {
                context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MAX_GRANULES_LESS_THAN_HIGHEST_ALLOCATED );
                context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000LL );
                return false;
            }
        }

        pInfo->m_MaxGranulesSpecified = true;
    }

    return true;
}


//  checkDiskPlacement()
//
//  Validates the placement field (if it exists).
//  Generally, we just ignore this (although we do post a warning if it is specified)
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      pActivity:          pointer to controlling Activity
//      catalogRequest:     true if we got here from @CAT
//
//  Returns:
//      false if we detect an error, else true
bool
FacilitiesManager::checkDiskPlacement
(
    const Context&          context,
    const FieldList&        additionalFields,
    const bool              catalogRequest
)
{
    if ( (additionalFields.size() > DISK_INITIAL_RESERVE_FIELD)
        && (additionalFields[DISK_INITIAL_RESERVE_FIELD].size() > DISK_PLACEMENT_SUBFIELD )
        && (additionalFields[DISK_INITIAL_RESERVE_FIELD][DISK_PLACEMENT_SUBFIELD].size() > 0) )
    {
        if ( catalogRequest )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_UNITS_AND_LOGICAL_SUBFIELDS_NOT_ALLOWED_FOR_CAT );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000ll );
            return false;
        }

        //  The additional following would be emitted in various situations if we honored the placement field.
        //      E:245333 Illegal character(s) in placement field.
        //      E:252433 Illegal syntax in placement subfield.

        //  If there's a placement specified, ignore it (and emit a warning)
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_PLACEMENT_FIELD_IGNORED );
    }

    return true;
}


//  checkForExistingCycle()
//
//  Check to see whether the indicated file cycle exists.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      fileSpecification:  user's file specification
//      fileSetInfo:        FileSetInfo containing read/write key information from the MFD
//      asgExisting:        if true, called for @ASG,A; if false, called for @ASG,C or @ASG,U or @CAT
//
//  Returns:
//      true if processing should continue (i.e, we detect no errors),
//      false if processing should stop
bool
FacilitiesManager::checkForExistingCycle
(
    const Context&                  context,
    const FileSpecification&        fileSpecification,
    const MFDManager::FileSetInfo&  fileSetInfo,
    const bool                      asgExisting
)
{
    INDEX ex;
    bool exists = findExistingCycleEntryIndex( fileSpecification, fileSetInfo, &ex );
    bool error = false;

    if ( exists && !asgExisting )
    {
        context.m_pResult->m_StatusBitMask.logicalOr( 0500000000000ll );
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_CATALOGED );
        error = true;
    }
    else if ( !exists && asgExisting )
    {
        context.m_pResult->m_StatusBitMask.logicalOr( 0400010000000ll );
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_NOT_CATALOGED );
        error = true;
    }

    return !error;
}


//  checkKeys()
//
//  Make sure the read and write keys are correct between the file specification and the given FileSetInfo.
//  We'll put the appropriate status codes and bits into pResult, but there may be other codes which need to
//  be appended also - caller must do this.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      fileSpecification:  user's file specification
//      fileSetInfo:        FileSetInfo containing read/write key information from the MFD
//      catalogFlag:        true if this is called during an @CAT operation...
//                              if so, we ignore pReadDisabled and pWriteDisabled
//      pReadDisabled:      we set this true if a read key exists, and was not specified
//      pWriteDisabled:     we set this true if a write key exists, and was not specified
//
//  Returns:
//      true if processing should continue (i.e, we detect no errors),
//      false if processing should stop
bool
FacilitiesManager::checkKeys
(
    const Context&                  context,
    const FileSpecification&        fileSpecification,
    const MFDManager::FileSetInfo&  fileSetInfo,
    const bool                      catalogFlag,
    bool* const                     pReadDisabled,
    bool* const                     pWriteDisabled
)
{
    //  Allow everything if the run is privileged and the file set is not guarded.
    if ( context.m_pSecurityContext->canBypassFileKeys() && !fileSetInfo.m_Guarded )
        return true;

    //  If the caller provided a read key and no key exists, or the wrong key was given, stop here.
    if ( fileSpecification.m_ReadKey.size() > 0 )
    {
        if ( fileSetInfo.m_ReadKey.size() == 0)
        {
            context.m_pResult->m_StatusBitMask.logicalOr( 0401000000000ll );
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_NOT_CATALOGED_WITH_READ_KEY );
            return false;
        }
        if ( fileSpecification.m_ReadKey.compareNoCase( fileSetInfo.m_ReadKey ) )
        {
            context.m_pResult->m_StatusBitMask.logicalOr( 0401000000000ll );
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_INCORRECT_READ_KEY );
            return false;
        }
    }

    //  If the caller provided a write key and no key exists, or the wrong key was given, stop here.
    if ( fileSpecification.m_WriteKey.size() > 0 )
    {
        if ( fileSetInfo.m_WriteKey.size() == 0)
        {
            context.m_pResult->m_StatusBitMask.logicalOr( 0401000000000ll );
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_NOT_CATALOGED_WITH_WRITE_KEY );
            return false;
        }
        if ( fileSpecification.m_WriteKey.compareNoCase( fileSetInfo.m_WriteKey ) )
        {
            context.m_pResult->m_StatusBitMask.logicalOr( 0401000000000ll );
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_INCORRECT_WRITE_KEY );
            return false;
        }
    }

    //  If a read key exists and none was given..., set read-disabled and post a warning.
    if ( (fileSetInfo.m_ReadKey.size() > 0) && (fileSpecification.m_ReadKey.size() == 0) )
    {
        //  For @CAT, reject the attempt
        if ( catalogFlag )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED );
            context.m_pResult->m_StatusBitMask.logicalOr( 0401000000000ll );
            return false;
        }

        //  For @ASG, set read-disabled and post a warning
        else
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_KEY_EXISTS );
            context.m_pResult->m_StatusBitMask.logicalOr( 0000100000000ll );
            *pReadDisabled = true;
        }
    }

    //  If a write key exists and none was given...
    if ( (fileSetInfo.m_WriteKey.size() > 0) && (fileSpecification.m_WriteKey.size() == 0) )
    {
        //  For @CAT, reject the attempt
        if ( catalogFlag )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED );
            context.m_pResult->m_StatusBitMask.logicalOr( 0400400000000ll );
            return false;
        }

        //  For @ASG, set write-disabled and post a warning
        //  Normal EXEC has this as an error, not a warning.  We're nicer.
        else
        {
            context.m_pResult->m_StatusBitMask.logicalOr( 0000200000000ll );
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_WRITE_KEY_EXISTS );
            *pWriteDisabled = true;
        }
    }

    return true;
}


//  checkOptionConflicts()
//
//  Checks the given options against a mask of mutually-exclusive options,
//  to see whether any mutually exclusive options were provided.
//  If so, we create a status code instance in the given Result object.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      optionsGiven:           bit mask of options specified
//      optionsAllowed:         bit mask of mutually-exclusive options
//
//  Returns:
//      true if no mutually-exclusive options were given, else false
bool
FacilitiesManager::checkOptionConflicts
(
    const Context&          context,
    const UINT32            optionsGiven,
    const UINT32            mutuallyExclusiveOptions
)
{
    UINT32 optResult = optionsGiven & mutuallyExclusiveOptions;
    UINT32 optBit = OPTB_A;
    char option = 'A';
    char firstGivenOption = 0;
    for ( INDEX count = 0; count < 26; ++count )
    {
        UINT32 set = optBit & optResult;
        if ( set > 0 )
        {
            if ( firstGivenOption == 0 )
                firstGivenOption = option;
            else
            {
                std::stringstream strm1;
                strm1 << firstGivenOption;
                std::stringstream strm2;
                strm2 << option;
                context.m_pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ILLEGAL_OPTION_COMBINATION, strm1.str(), strm2.str() ) );
                context.m_pResult->m_StatusBitMask.setW( 0600000000000 );
                return false;
            }
        }

        optBit >>= 1;
        option += 1;
    }

    return true;
}


//  checkOptions()
//
//  Checks the given options against a mask of allowable options.
//
//  Parameters:
//      optionsGiven:           bit mask of options specified
//      optionsAllowed:         bit mask of options allowed
//      pResult:                where we store particular codes if the check fails
//                                  if not specified, we do not generate status code instances.
//
//  Returns:
//      true if all given options are in the list of allowed options, else false
bool
FacilitiesManager::checkOptions
(
    const UINT32            optionsGiven,
    const UINT32            optionsAllowed,
    Result* const           pResult
)
{
    const UINT32 badOptions = optionsGiven & (~optionsAllowed);
    UINT32 optBit = OPTB_A;
    char option = 'A';
    for ( INDEX count = 0; count < 26; ++count )
    {
        UINT32 set = optBit & badOptions;
        if ( set )
        {
            if ( pResult )
            {
                std::stringstream varStrm;
                varStrm << option;
                pResult->m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ILLEGAL_OPTION, varStrm.str() ) );
                pResult->m_StatusBitMask.setW( 0600000000000 );
            }
            return false;
        }

        optBit >>= 1;
        option += 1;
    }

    return true;
}


//  checkMFDResult()
//
//  Some generic MFD result-handling code.
//  Based on MFD bad result status, we need to update the facilities result in one or another
//  of various codes...
bool
FacilitiesManager::checkMFDResult
(
    const MFDManager::Result        mfdResult,
    Result* const                   pFacilitiesResult
)
{
    if ( mfdResult.m_Status == MFDManager::MFDST_OUT_OF_SPACE )
    {
        pFacilitiesResult->m_StatusCodeInstances.push_back( FSCODE_MASS_STORAGE_OVERFLOW );
        pFacilitiesResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }
    else if ( mfdResult.m_Status == MFDManager::MFDST_TERMINATING )
    {
        pFacilitiesResult->m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        pFacilitiesResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }
    else if ( mfdResult.m_Status == MFDManager::MFDST_FILE_ON_PRINT_QUEUE )
    {
        pFacilitiesResult->m_StatusCodeInstances.push_back( FSCODE_FILE_ON_PRINT_QUEUE );
        pFacilitiesResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }
    else if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
    {
        //  IO error is not correct, but it's the best we've got
        pFacilitiesResult->m_StatusCodeInstances.push_back( FSCODE_IO_ERROR_ENCOUNTERED_ON_MFD );
        pFacilitiesResult->m_StatusBitMask.logicalOr( 0600000000000ll );
        return false;
    }

    return true;
}


//  checkTempFileAssigned()
//
//  Given a full filespecification (i.e., includes given or effective qualifier),
//  we check to see if a FacilityItem already exists.
//  This is for comparing a temporary file assignment against the existing set of assigned files.
//
//  First we compare the filename to see if we have at least a file name match.
//  If so, we then compare the qualifiers.  If that also matches, then:
//      If the fileSpec has no cycle, we have a match.
//          @ASG,T qual*file(5).
//          @ASG,T qual*file.       . this matches the previous line
//      If the fileSpec and the FacItem both have a relative cycle given, compare them.
//      If the fileSpec and the FacItem both have an absolute cycle given, compare them.
//
//  Then, if we determine that the file is already assigned, we do some common compatibility checks.
//  These checks are not specific to tape or disk, so those additional checks must be done by the caller.
//  This does mean that we might stick some innocuous warning codes into pResult when it will subsequently
//  get some error... This should be okay.
//
//  Parameters:
//      context:            context object indicating requesting Activity, SecurityContext, and affected RunInfo objects
//      fileSpec:           File specification for the pending @ASG,T with qualifier given
//                              or determined by getEffectiveFileSpecification()
//      pItFacItem:         where we store the iterator to the facitem, if the item was found
//      pResult:            Result object which we might update with warning(s) and/or error(s)
//
//  Returns:
//      true if an existing FacilityItem was found, else false
bool
FacilitiesManager::checkTempFileAssigned
(
    const Context&              context,
    const FileSpecification&    fileSpec,
    CITFACITEMS* const          pItFacItem
)
{
    bool fileNameMatch = false;
    bool found = false;
    const FACITEMS& facItems = context.m_pRunInfo->getFacilityItems();
    CITFACITEMS itFacItem = facItems.begin();
    while ( itFacItem != facItems.end() )
    {
        StandardFacilityItem* pFacItem = dynamic_cast<StandardFacilityItem*>(itFacItem->second);
        if ( pFacItem->getFileName().compareNoCase( fileSpec.m_FileName ) == 0 )
        {
            fileNameMatch = true;

            //  Filename matches, check for qualifier match.
            //  If qualifier matches, check for cycle match - if not, set fileOnlyMatch flag
            if ( pFacItem->getQualifier().compareNoCase( fileSpec.m_Qualifier ) == 0 )
            {
                if ( fileSpec.m_AbsoluteCycleSpecified )
                {
                    if ( pFacItem->getAbsoluteFileCycleExists()
                        && ( pFacItem->getAbsoluteFileCycle() == fileSpec.m_Cycle ) )
                    {
                        found = true;
                        break;
                    }
                }
                else if ( fileSpec.m_RelativeCycleSpecified )
                {
                    if ( pFacItem->getRelativeFileCycleSpecified()
                        && ( pFacItem->getRelativeFileCycle() == fileSpec.m_Cycle ) )
                    {
                        found = true;
                        break;
                    }
                }
                else
                {
                    found = true;
                    break;
                }
            }
        }

        ++itFacItem;
    }

    if ( found )
    {
        //  File is already assigned.  Make sure it's not a cataloged (or being-cataloged) file.
        StandardFacilityItem* psfi = dynamic_cast<StandardFacilityItem*>( itFacItem->second );
        if ( (psfi == 0) || ( psfi && !psfi->getTemporaryFileFlag() ) )
        {
            context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE );
            context.m_pResult->m_StatusBitMask.logicalOr( 0600000000000l );
            return false;
        }

        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_IS_ALREADY_ASSIGNED );
        context.m_pResult->m_StatusBitMask.logicalOr( 0100000000000ll );

        *pItFacItem = itFacItem;
    }
    else if ( fileNameMatch )
        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_FILE_NAME_NOT_UNIQUE );

    return found;
}


//  compareFileSpecificationToFacilityItem()
//
//  Used by routines which need to compare the one against the other
bool
FacilitiesManager::compareFileSpecificationToFacilityItem
(
    const FileSpecification&    fileSpecification,
    const FacilityItem* const   pFacilityItem
)
{
    //  Compare qual and file
    if ( fileSpecification.m_Qualifier.compareNoCase( pFacilityItem->getQualifier() ) != 0 )
        return false;
    if ( fileSpecification.m_FileName.compareNoCase( pFacilityItem->getFileName() ) != 0 )
        return false;

    //  If this is a non-standard facility item, we're done.
    const StandardFacilityItem* psfi = dynamic_cast<const StandardFacilityItem*>( pFacilityItem );
    if ( psfi == 0 )
        return true;

    //  Compare absolute file cycles, if appropriate
    if ( fileSpecification.m_AbsoluteCycleSpecified )
        return ( fileSpecification.m_Cycle == psfi->getAbsoluteFileCycle() );

    //  Compare relative file cycles, if appropriate
    if ( fileSpecification.m_RelativeCycleSpecified )
        return psfi->getRelativeFileCycleSpecified() && ( fileSpecification.m_Cycle == psfi->getRelativeFileCycle() );

    //  No cycle problems - this is a match
    return true;
}


//  findExistingCycleEntryIndex()
//
//  Find the CycleInfo object corresponding to the indicated cycle (in the filespec)
//  within the given FileSetInfo object
//
//  Parameters:
//      fileSpecification:  user's file specification
//      fileSetInfo:        FileSetInfo containing read/write key information from the MFD
//      pIndex:             where we store the index of the discovered entry, if found
//
//  Returns:
//      pointer to const CycleInfo object if found, else 0
bool
FacilitiesManager::findExistingCycleEntryIndex
(
    const FileSpecification&        fileSpecification,
    const MFDManager::FileSetInfo&  fileSetInfo,
    INDEX* const                    pIndex
)
{
    if ( fileSpecification.m_RelativeCycleSpecified )
    {
        for ( INDEX ex = 0; ex < fileSetInfo.m_CycleEntries.size(); ++ex )
        {
            if ( fileSetInfo.m_CycleEntries[ex].m_Exists &&
                ( fileSetInfo.m_CycleEntries[ex].m_RelativeCycle == fileSpecification.m_Cycle ) )
            {
                *pIndex = ex;
                return true;
            }
        }
    }
    else if ( fileSpecification.m_AbsoluteCycleSpecified )
    {
        for ( INDEX ex = 0; ex < fileSetInfo.m_CycleEntries.size(); ++ex )
        {
            if ( fileSetInfo.m_CycleEntries[ex].m_Exists &&
                ( fileSetInfo.m_CycleEntries[ex].m_AbsoluteCycle == fileSpecification.m_Cycle ) )
            {
                *pIndex = ex;
                return true;
            }
        }
    }
    else
    {
        //  No cycle specification - this implies relative cycle zero, which certainly exists.
        for ( INDEX ex = 0; ex < fileSetInfo.m_CycleEntries.size(); ++ex )
        {
            if ( fileSetInfo.m_CycleEntries[ex].m_Exists )
            {
                *pIndex = ex;
                return true;
            }
        }
    }

    return false;
}


//  findStatusCodeInfo()
//
//  Searches the static table of StatusCodeInfo structs for the one containing the indicated StatusCode.
//  Purposely aborts if struct is not found, in order to call attention to the situation where we
//  add a new StatusCode, and forget to add the appropriate struct.
const FacilitiesManager::StatusCodeInfo*
FacilitiesManager::findStatusCodeInfo
(
    const StatusCode        statusCode
)
{
    for ( INDEX sx = 0; sx < m_StatusCodeInfoTableSize; ++sx )
        if ( m_StatusCodeInfoTable[sx].m_StatusCode == statusCode )
            return &m_StatusCodeInfoTable[sx];

    //  This should never happen
    return 0;
}


//  getEffectiveFileSpecification()
//
//  Given an internal filename, we (may) walk any existing chain of use-names until we reach an end.
//  This should only be called when the caller is trying to translate a filename-only FileSpecification
//  into an actual qualifier and filename (and optional relative or absolute file cycle).
//
//  Parameters:
//      pRunInfo:                   pointer to the RunInfo of interest
//      originalSpecification:      beginning file specification which does not have a qualifier specified
//      pEffectiveSpecification:    pointer to effective specification which we populate base on the
//                                      original specification and information in the given RunInfo.
void
FacilitiesManager::getEffectiveFileSpecification
(
    const RunInfo* const        pRunInfo,
    const FileSpecification&    originalSpecification,
    FileSpecification* const    pEffectiveSpecification
)
{
    //  Set up the destination specification
    *pEffectiveSpecification = originalSpecification;

    //  If there's nothing but a filename in the original, check NameItem container
    //  to see whether this is an internal use name for something else.
    if ( originalSpecification.isFileNameOnly() )
    {
        const RunInfo::NameItem* pNameItem = pRunInfo->getNameItem( originalSpecification.m_FileName );
        if ( pNameItem )
        {
            //  Copy the filespec from the name item to the effective spec.
            *pEffectiveSpecification = pNameItem->m_FileSpecification;

            //  If the name item refers to an actual facitem, use information from the facitem to
            //  overwrite certain fields in the effective specification.
            if ( pNameItem->m_FacilityItemIdentifier != FacilityItem::INVALID_IDENTIFIER )
            {
                const FacilityItem* pfi = pRunInfo->getFacilityItem( pNameItem->m_FacilityItemIdentifier );
                const StandardFacilityItem* psfi = dynamic_cast<const StandardFacilityItem*>( pfi );
                pEffectiveSpecification->m_Qualifier = pfi->getQualifier();
                pEffectiveSpecification->m_QualifierSpecified = true;

                if ( psfi )
                {
                    pEffectiveSpecification->m_RelativeCycleSpecified = false;
                    pEffectiveSpecification->m_AbsoluteCycleSpecified = true;
                    pEffectiveSpecification->m_Cycle = psfi->getAbsoluteFileCycle();
                }
            }
        }
    }

    //  Resolve the qualifier as necessary
    if ( pEffectiveSpecification->m_Qualifier.size() == 0 )
    {
        pEffectiveSpecification->m_Qualifier = resolveQualifier( pRunInfo, *pEffectiveSpecification );
        if ( pEffectiveSpecification->m_Qualifier.size() > 0 )
            pEffectiveSpecification->m_QualifierSpecified = true;
    }
}


//  isFileAssigned()
//
//  Checks the given RunInfo object to see if a file is already assigned to the run.
//  If an absolute file cycle is given, we search for an existing fac item with a matching absolute cycle number.
//  If a relative file cycle is given, we search for an exising fac item with a matching relative cycle number.
//  If neither are specified, we look for any matching qual/file combination.
//
//  Temporary fac items are never considered.
//
//  Parameters:
//      fileSpecification:      file spec of interest
//      runInfo:                RunInfo object to be searched
//      pIterator:              where we store a FACITEMS const iterator if found
//
//  Returns:
//      true if we find a fac item (populating pIterator), else false
bool
FacilitiesManager::isFileAssigned
(
    const FileSpecification& fileSpecification,
    const RunInfo&          runInfo,
    CITFACITEMS* const      pIterator
)
{
    const FACITEMS& facItems = runInfo.getFacilityItems();
    bool found = false;
    for ( CITFACITEMS itFacItem = facItems.begin(); !found && (itFacItem != facItems.end()); ++itFacItem )
    {
        if ( compareFileSpecificationToFacilityItem( fileSpecification, itFacItem->second ) )
        {
            *pIterator = itFacItem;
            found = true;
            break;
        }
    }

    return found;
}


//  logMFDError()
//
//  Common code for logging information helpful for debugging MFD-related exec stops
void
FacilitiesManager::logMFDError
(
    const std::string&         locus,
    const FileSpecification&   fileSpecification,
    const MFDManager::Result&  mfdResult
)
{
    std::stringstream strm;
    strm << locus << " File=";
    if ( fileSpecification.m_QualifierSpecified )
        strm << fileSpecification.m_Qualifier << "*";
    strm << fileSpecification.m_FileName
            << " MFDResult=" << MFDManager::getResultString( mfdResult );
    SystemLog::write( strm.str() );
}


//  logMFDError()
//
//  Common code for logging information helpful for debugging MFD-related exec stops
void
FacilitiesManager::logMFDError
(
    const Context&             context,
    const std::string&         locus,
    const FileSpecification&   fileSpecification,
    const MFDManager::Result&  mfdResult
)
{
    std::stringstream strm;
    strm << locus << " Runid=" << context.m_pRunInfo->getActualRunId() << " File=";
    if ( fileSpecification.m_QualifierSpecified )
        strm << fileSpecification.m_Qualifier << "*";
    strm << fileSpecification.m_FileName
            << " MFDResult=" << MFDManager::getResultString( mfdResult );
    SystemLog::write( strm.str() );
}


//  logMFDError()
//
//  Common code for logging information helpful for debugging MFD-related exec stops
void
FacilitiesManager::logMFDError
(
    const Context&             context,
    const std::string&         locus,
    const FacilityItem* const  pFacilityItem,
    const MFDManager::Result&  mfdResult
)
{
    std::stringstream strm;
    strm << locus << " Runid=" << context.m_pRunInfo->getActualRunId()
        << " File=" << pFacilityItem->getQualifier() << "*" << pFacilityItem->getFileName()
        << " MFDResult=" << MFDManager::getResultString( mfdResult );
    SystemLog::write( strm.str() );
}


//  resolveQualifier()
//
//  Given a file spec, we return the effective qualifier for that filespec.
//  If specified, the resolved qualifier is the specified qualifier.
//  If not specified
//      If asterisk is given, use implied qualifier
//      Else use default qualifier
//
//  Parameters:
//      pRunInfo:           pointer to the RunInfo of interest
//      pFileSpecification: pointer to FileSpecification object of interest
const SuperString
FacilitiesManager::resolveQualifier
(
    const RunInfo* const        pRunInfo,
    const FileSpecification&    fileSpecification
)
{
    if ( fileSpecification.m_Qualifier.size() == 0 )
    {
        if ( fileSpecification.m_QualifierSpecified )
            return pRunInfo->getImpliedQualifier();
        else
            return pRunInfo->getDefaultQualifier();
    }

    return fileSpecification.m_Qualifier;
}



//  constructors, destructors

FacilitiesManager::FacilitiesManager
(
    Exec* const         pExec
)
:ExecManager( pExec ),
m_pMFDManager( dynamic_cast<MFDManager*>( pExec->getManager( Exec::MID_MFD_MANAGER ) ) )
{
    //  Load configuration tags
    m_DefaultMaxGranules = 0;
    m_ReleaseUnusedReserve = true;
    m_PrivateByAccount = true;
    m_RejectZOptionOnBatch = false;

    //  Build EquipmentModel table - there's only one table, since there's only one model (for now)
    m_EquipmentModels[ModelEMFSD.m_pModelName] = &ModelEMFSD;
    m_EquipmentModels[ModelEMFST.m_pModelName] = &ModelEMFST;

    //  Build EquipmentType table (eventually we may pull this from the configurator)
    const EquipmentModel* pemTapeTable[] = { &ModelEMFST };
    const COUNT pemTapeTableSize = sizeof(pemTapeTable) / sizeof(EquipmentModel*);
    m_EquipmentTypes.push_back( new EquipmentType( "T",     ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  any tape
    m_EquipmentTypes.push_back( new EquipmentType( "U9",    ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T, any density
    m_EquipmentTypes.push_back( new EquipmentType( "U9H",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T, 800BPI
    m_EquipmentTypes.push_back( new EquipmentType( "U9V",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T, 1600BPI
    m_EquipmentTypes.push_back( new EquipmentType( "U9S",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T, 6250BPI
    m_EquipmentTypes.push_back( new EquipmentType( "26N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "28N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "32N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "34N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "36N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "45N",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  2145 9T unit
    m_EquipmentTypes.push_back( new EquipmentType( "22D",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  Uniservo 22 dual-dens 9T
    m_EquipmentTypes.push_back( new EquipmentType( "24D",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  Uniservo 24 dual-dens 9T
    m_EquipmentTypes.push_back( new EquipmentType( "30D",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  Uniservo 30 dual-dens 9T
    m_EquipmentTypes.push_back( new EquipmentType( "HIC",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  All half-inch cart
    m_EquipmentTypes.push_back( new EquipmentType( "HICL",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  All 18T HIC
    m_EquipmentTypes.push_back( new EquipmentType( "U47",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  4780 18T
    m_EquipmentTypes.push_back( new EquipmentType( "U47L",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  4780 18T ACS
    m_EquipmentTypes.push_back( new EquipmentType( "LCART", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  4780 18T ACS
    m_EquipmentTypes.push_back( new EquipmentType( "NLCART",ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  4780 18T
    m_EquipmentTypes.push_back( new EquipmentType( "U47NL", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  4780 18T
    m_EquipmentTypes.push_back( new EquipmentType( "HICM",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  All 36T HIC
    m_EquipmentTypes.push_back( new EquipmentType( "U47M",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  36T HIC
    m_EquipmentTypes.push_back( new EquipmentType( "U47LM", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  36T HIC ACS
    m_EquipmentTypes.push_back( new EquipmentType( "HIC40", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  Uniservo 40 18T
    m_EquipmentTypes.push_back( new EquipmentType( "HIC51", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  SCSI 36T
    m_EquipmentTypes.push_back( new EquipmentType( "HIC52", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  SCSI/SBCON 36T
    m_EquipmentTypes.push_back( new EquipmentType( "HICSL", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  SCSI low dens
    m_EquipmentTypes.push_back( new EquipmentType( "DLT70", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  SCSL DLT
    m_EquipmentTypes.push_back( new EquipmentType( "HIS98", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  HIC serpentine
    m_EquipmentTypes.push_back( new EquipmentType( "HIS98C",ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  HIC serpentine
    m_EquipmentTypes.push_back( new EquipmentType( "HIS98D",ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  HIC serpentine
    m_EquipmentTypes.push_back( new EquipmentType( "HIS98B",ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  HIC serpentine
    m_EquipmentTypes.push_back( new EquipmentType( "T10KA", ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  HIC serpentine
    m_EquipmentTypes.push_back( new EquipmentType( "LTO",   ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  All Linear Tape Open carts
    m_EquipmentTypes.push_back( new EquipmentType( "LTO3",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  LTO Gen 3
    m_EquipmentTypes.push_back( new EquipmentType( "LTO4",  ECAT_TAPE,  FACETG_TAPE,   pemTapeTableSize,   pemTapeTable ) );   //  LTO Gen 4

    const EquipmentModel* pemDiskTable[] = { &ModelEMFSD };
    const COUNT pemDiskTableSize = sizeof(pemDiskTable) / sizeof(EquipmentModel*);
    m_EquipmentTypes.push_back( new EquipmentType( "F",     ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F2",    ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F3",    ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F4",    ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F14",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F14C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F14D",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F17",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F24",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F25",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F30",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F33",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F34",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F36",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F40",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F50",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F50C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F50F",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F50M",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F51",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F53",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F54",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F59F",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F59M",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F60",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F63",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F70C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F70F",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F70M",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F80C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F80M",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F81",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F81C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F94",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "F94C",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "FCS",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "FMD",   ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "FMEM",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "FSCSI", ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "FSSD",  ECAT_DISK,  FACETG_SECTOR_DISK,    pemDiskTableSize,   pemDiskTable ) );

    m_EquipmentTypes.push_back( new EquipmentType( "D",     ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D2",    ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D3",    ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D4",    ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D14",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D14C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D14D",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D17",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D24",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D25",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D30",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D33",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D34",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D36",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D40",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D50",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D50C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D50F",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D50M",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D51",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D53",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D54",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D59F",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D59M",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D60",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D63",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D70C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D70F",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D70M",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D80C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D80M",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D81",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D81C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D94",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "D94C",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "DCS",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "DMD",   ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "DMEM",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "DSCSI", ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
    m_EquipmentTypes.push_back( new EquipmentType( "DSSD",  ECAT_DISK,  FACETG_WORD_DISK,      pemDiskTableSize,   pemDiskTable ) );
}



//  public methods

//  asg()
//
//  General entry point, for all attempts to assign a file.
//  Special handling:
//      For batch files, Z option (do not wait on availability) is never honored.
//          Depending upon configuration, it is either ignored with a warning, or the run is err'd.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity - only for posting contingencies (if necessary)
//      pSecurityContext:   pointer to security context - it will be updated if we newly-assign sys$*dloc$ with no inhibits
//      pRunInfo:           pointer to RunInfo which is to be affected (might not be the RunInfo associated with the Activity)
//      options:            options given on the @ASG
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved -- so CSInterpreter doesn't do much
//                              with them - it's up to us to validate them.
FacilitiesManager::Result
FacilitiesManager::asg
(
    Activity* const             pActivity,
    SecurityContext* const      pSecurityContext,
    RunInfo* const              pRunInfo,
    const UINT32                options,
    const FileSpecification&    fileSpecification,
    const FieldList&            additionalFields
)
{
    Result result;
    Context context( pActivity, pSecurityContext, pRunInfo, &result );

    {//TODO:DEBUG
        std::cout << "FacilitiesManager::asg() " << fileSpecification << " " << additionalFields << std::endl;
    }

    //  Is this Z option and batch?  Should we reject it?
    if ( !checkZOption( context, options ) )
        return result;

    //  Only one of A,C,T, and U are accepted
    if ( !checkOptionConflicts( context, options, OPTB_A | OPTB_C | OPTB_T | OPTB_U ) )
        return result;

    //  Because we don't have owned files, A and R combination is not allowed
    if ( !checkOptionConflicts( context, options, OPTB_A | OPTB_R ) )
        return result;

    //  Call a sub-function to do the work.  Do it in a loop to honor hold conditions.
    //  Note that we never allow an FSCODE_HELD status to make it back to the user.
    //  Either we handle them here, or we convert them to FSCODE_xxx_REJECTED_Z codes before returning.
    bool asgResult = false;
    time_t  initialTime;
    time( &initialTime );
    time_t quantumTime = initialTime;

    bool firstIteration = true;
    while ( !pActivity->isTerminating() )
    {
        lock();
        pRunInfo->attach();

        //  Determine effective qualifier and filename.
        FileSpecification effectiveSpec;
        getEffectiveFileSpecification( pRunInfo, fileSpecification, &effectiveSpec );

        if ( options & OPTB_A )
            asgResult = asgExisting( context, options, effectiveSpec, additionalFields  );
        else if (options & (OPTB_C | OPTB_U))
            asgResult = asgCatalog( context, options, effectiveSpec, additionalFields );
        else if ( options & OPTB_T )
            asgResult = asgTemporary( context, options, effectiveSpec, additionalFields );
        else
            asgResult = asgUnspecified( context, options, effectiveSpec, additionalFields );

        pRunInfo->detach();
        unlock();

        //  Check for hold condition.  If none, we're done here.
        if ( !result.containsHoldStatusCode() )
            break;

        //  If Z option and non-batch, convert hold condition to error condition, and we're still done.
        if ( ( (options & OPTB_Z) != 0 ) && !pRunInfo->isBatch() )
        {
            result.convertHoldStatusCodes();
            result.m_StatusBitMask.logicalOr( 0400000000000LL );
            return result;
        }

        //  If we have waited for a 2-minute quantum, post message(s) to PRINT$ and reset quantum timer.
        //  Only do this for control mode.
        //  The dynamic_cast always works, since non UserRunInfo can never be in control mode.
        if ( pRunInfo->inControlMode() )
        {
            UserRunInfo* prui = dynamic_cast<UserRunInfo*>( pRunInfo );
            time_t currentTime;
            time( &currentTime );
            if ( firstIteration || (difftime( currentTime, quantumTime ) > 120) )
            {
                quantumTime = currentTime;
                time_t waitTime = static_cast<time_t>( difftime( currentTime, initialTime ) );
                std::stringstream waitStrm;
                waitStrm << std::setw( 2 ) << std::setfill( '0' ) << static_cast<COUNT>(waitTime / 60);

                for ( INDEX sx = 0; sx < result.m_StatusCodeInstances.size(); ++sx )
                {
                    //  All HELD messages have two variable placeholders -
                    //  the first is run-id, the second is elapsed time in minutes.
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings.resize(2);
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings[0] = prui->getActualRunId();
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings[1] = waitStrm.str();
                    prui->attach();
                    prui->postToPrint( result.m_StatusCodeInstances[sx].getString() );
                    prui->detach();
                }
            }
        }

        //  Clear result packet, wait a little bit, and then retry.
        //  Don't forget to re-poll for Z option - it won't fail now, but it might still emit a warning,
        //  which we will clear out (and don't want to lose).
        result.clear();
        checkZOption( context, options );

        miscSleep( 250 );
        pRunInfo->attach();
        pRunInfo->incrementResourceWaitMilliseconds( 250 );
        pRunInfo->detach();

        firstIteration = false;
    }

    //  Is the activity ending?  This could be the result of an E or X keyin, or an @@X statement.
    if ( pActivity->isTerminating() )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        result.m_StatusBitMask.logicalOr( 0600000000000ll );
    }

    //  Look for readkey/writekey errors, and abort the run as necessary
    if ( result.containsStatusCode( FSCODE_INCORRECT_READ_KEY )
        || result.containsStatusCode( FSCODE_INCORRECT_WRITE_KEY ) )
    {
        m_pExec->abortRunFacilities( context.m_pRunInfo );
    }

    //  Done
    if ( asgResult )
        result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_COMPLETE, "ASG" ) );

    return result;
}


//  cat()
//
//  General entry point, for all attempts to catalog a file.
//  Special handling:
//      For batch files, Z option (do not wait on availability) is never honored.
//          Depending upon configuration, it is either ignored with a warning, or the run is err'd.
//
//  Parameters:
//      pActivity:          pointer to controlling Activity - only for posting contingencies (if necessary)
//      pSecurityContext:   pointer to security context - it will be updated if we newly-assign sys$*dloc$ with no inhibits
//      pRunInfo:           pointer to RunInfo which is to be affected (might not be the RunInfo associated with the Activity)
//      options:            options given on the @CAT
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
//      additionalFields:   additional fields beyond the file specification - the semantics of these fields and
//                              subfields are dependent upon (to some extent) the options, and more directly,
//                              to the equipment type of the file involved -- so CSInterpreter doesn't do much
//                              with them - it's up to us to validate them.
FacilitiesManager::Result
FacilitiesManager::cat
(
    Activity* const                 pActivity,
    SecurityContext* const          pSecurityContext,
    RunInfo* const                  pRunInfo,
    const UINT32                    options,
    const FileSpecification&        fileSpecification,
    const FieldList&                additionalFields
)
{
    Result result;
    Context context( pActivity, pSecurityContext, pRunInfo, &result );

    {//TODO:DEBUG
        std::cout << "FacilitiesManager::cat() " << fileSpecification << " " << additionalFields << std::endl;
    }

    //  Is this Z option and batch?  Should we reject it?
    if ( !checkZOption( context, options ) )
        return result;

    //  If there's an equipment type given, validate it.
    const EquipmentType* pEquipmentType = checkEquipmentType( context, additionalFields );
    if ( !pEquipmentType && context.m_pResult->containsErrorStatusCode() )
        return result;

    //  Loop for honoring hold conditions...
    //  Note that we never allow an FSCODE_HELD status to make it back to the user.
    //  Either we handle them here, or we convert them to FSCODE_xxx_REJECTED_Z codes before returning.
    bool catResult = false;
    time_t  initialTime;
    time( &initialTime );
    time_t quantumTime = initialTime;

    bool firstIteration = true;
    while ( !pActivity->isTerminating() )
    {
        lock();
        pRunInfo->attach();

        //  Determine effective file specification.
        FileSpecification effectiveSpec;
        getEffectiveFileSpecification( pRunInfo, fileSpecification, &effectiveSpec );

        //  See whether a file set exists
        MFDManager::FileSetInfo fileSetInfo;
        MFDManager::Result mfdResult = m_pMFDManager->getFileSetInfo( effectiveSpec.m_Qualifier,
                                                                      effectiveSpec.m_FileName,
                                                                      &fileSetInfo );
        if ( ( mfdResult.m_Status != MFDManager::MFDST_NOT_FOUND )
            && ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL ) )
        {
            unlock();
            checkMFDResult( mfdResult, &result );
            logMFDError( context, "FacilitiesManager::cat()", effectiveSpec, mfdResult );
            return result;
        }

        bool fileSetExists = ( mfdResult.m_Status == MFDManager::MFDST_SUCCESSFUL );

        //  Call an appropriate subfunction
        if ( !pEquipmentType )
            catResult = catUnknown( context, options, additionalFields, &effectiveSpec, fileSetExists ? &fileSetInfo : 0 );
        else if ( pEquipmentType->m_EquipmentCategory == ECAT_DISK )
            catResult = catDisk( context, options, additionalFields, &effectiveSpec, pEquipmentType, fileSetExists ? &fileSetInfo : 0 );
        else if ( pEquipmentType->m_EquipmentCategory == ECAT_TAPE )
            catResult = catTape( context, options, additionalFields, &effectiveSpec, pEquipmentType, fileSetExists ? &fileSetInfo : 0 );
        else
        {
            //  We should never be able to get here - all assign mnemonics are disk or tape.
            std::string logMsg = "FacilitiesManager::cat():Problem with equipment type:";
            logMsg += pEquipmentType->m_pMnemonic;
            SystemLog::write( logMsg );
            m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
            catResult = false;
        }

        pRunInfo->detach();
        unlock();

        //  Check for hold condition.  If none, we're done here.
        if ( !result.containsHoldStatusCode() )
            break;

        //  If Z option and non-batch, convert hold condition to error condition, and we're still done.
        if ( ( (options & OPTB_Z) != 0 ) && !pRunInfo->isBatch() )
        {
            result.convertHoldStatusCodes();
            result.m_StatusBitMask.logicalOr( 0400000000000LL );
            return result;
        }

        //  If we have waited for a 2-minute quantum, post message(s) to PRINT$ and reset quantum timer.
        //  Only do this for control mode.
        //  The dynamic_cast always works, since non UserRunInfo can never be in control mode.
        if ( pRunInfo->inControlMode() )
        {
            UserRunInfo* prui = dynamic_cast<UserRunInfo*>( pRunInfo );
            time_t currentTime;
            time( &currentTime );
            if ( firstIteration || ( difftime( currentTime, quantumTime ) > 120 ) )
            {
                quantumTime = currentTime;
                time_t waitTime = static_cast<time_t>( difftime( currentTime, initialTime ) );
                std::stringstream waitStrm;
                waitStrm << std::setw( 2 ) << std::setfill( '0' ) << static_cast<COUNT>(waitTime / 60);

                for ( INDEX sx = 0; sx < result.m_StatusCodeInstances.size(); ++sx )
                {
                    //  All HELD messages have two variable placeholders -
                    //  the first is run-id, the second is elapsed time in minutes.
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings.resize(2);
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings[0] = prui->getActualRunId();
                    result.m_StatusCodeInstances[sx].m_VariableInfoStrings[1] = prui->getActualRunId();
                    prui->attach();
                    prui->postToPrint( result.m_StatusCodeInstances[sx].getString() );
                    prui->detach();
                }
            }
        }

        //  Clear result packet, wait a little bit, and then retry.
        //  Don't forget to re-poll for Z option - it won't fail now, but it might still emit a warning,
        //  which we will clear out (and don't want to lose).
        result.clear();
        checkZOption( context, options );

        miscSleep( 250 );
        pRunInfo->attach();
        pRunInfo->incrementResourceWaitMilliseconds( 250 );
        pRunInfo->detach();

        firstIteration = false;
    }

    //  Is the activity ending?  This could be the result of an E or X keyin, or an @@X statement.
    if ( pActivity->isTerminating() )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_RUN_ABORTED );
        result.m_StatusBitMask.logicalOr( 0600000000000ll );
    }

    //  Look for readkey/writekey errors, and abort the run as necessary
    if ( result.containsStatusCode( FSCODE_INCORRECT_READ_KEY )
        || result.containsStatusCode( FSCODE_INCORRECT_WRITE_KEY ) )
    {
        m_pExec->abortRunFacilities( context.m_pRunInfo );
    }

    //  Done
    if ( catResult )
        result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_COMPLETE, "CAT" ) );
    return result;
}


//  dump()
void
FacilitiesManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      //not used
)
{
    stream << "FacilitiesManager ----------" << std::endl;
    //  Nothing interesting to dump.  Actually, nothing at all to dump.
}


//  free()
//
//  Releases a file, or a use-item
//
//  Parameters:
//      pActivity:          pointer to controlling Activity - only for posting contingencies (if necessary)
//      pSecurityContext:   pointer to security context - it will be updated if we release sys$*dloc$
//      pRunInfo:           pointer to RunInfo which is to be affected (might not be the RunInfo associated with the Activity)
//      options:            options given on the @CAT
//      fileSpecification:  the FileSpecification containing directory, qual, filename, cycle, and r/w keys
FacilitiesManager::Result
FacilitiesManager::free
(
    Activity* const             pActivity,
    SecurityContext* const      pSecurityContext,
    RunInfo* const              pRunInfo,
    const UINT32                options,
    const FileSpecification&    fileSpecification
)
{
    {//TODO:DEBUG
        std::cout << "FacilitiesManager::free() " << fileSpecification << std::endl;
    }

    Result result;
    Context context( pActivity, pSecurityContext, pRunInfo, &result );
    pRunInfo->attach();
    bool freeResult = false;

    //  Did the user specify A or B option?
    if ( options & (OPTB_A | OPTB_B) )
    {
        if ( !fileSpecification.isFileNameOnly() )
        {
            result.m_StatusCodeInstances.push_back( FSCODE_INTERNAL_NAME_IS_REQUIRED );
            result.m_StatusBitMask.logicalOr( 0600000000000ll );
        }
        else
            freeResult = freeInternalName( context, options, fileSpecification.m_FileName );
    }
    else
    {
        //  Determine effective file specification, then locate an appropriate FACITEM
        FileSpecification effectiveSpec;
        getEffectiveFileSpecification( pRunInfo, fileSpecification, &effectiveSpec );

        CITFACITEMS itfi;
        if ( !isFileAssigned( effectiveSpec, *context.m_pRunInfo, &itfi ) )
        {
            if ( options & OPTB_D )
            {
                result.m_StatusCodeInstances.push_back( FSCODE_ATTEMPT_TO_DELETE_VIA_FREE );
                result.m_StatusBitMask.logicalOr( 0600000000000ll );
            }
            else
            {
                result.m_StatusCodeInstances.push_back( FSCODE_FILE_NAME_NOT_KNOWN_TO_THIS_RUN );
                result.m_StatusBitMask.logicalOr( 0500000000000ll );
            }
        }
        else
        {
            FacilityItem* pFacilityItem = itfi->second;
            if ( options & OPTB_X )
            {
                //  Release exclusive use, not the whole file... and only for standard asg,a files.
                //  All other facitems succeed with no action.
                StandardFacilityItem* psfi = dynamic_cast<StandardFacilityItem*>( pFacilityItem );
                if ( psfi && psfi->getExistingFileFlag() )
                {
                    psfi->setExclusiveFlag( false );
                    MFDManager::Result mfdResult = m_pMFDManager->releaseExclusiveUse( context.m_pActivity, psfi->getMainItem0Addr() );
                    if ( mfdResult.m_Status == MFDManager::MFDST_IO_ERROR )
                        context.m_pResult->m_StatusCodeInstances.push_back( FSCODE_MFD_IO_ERROR_DURING_FREE );
                    else
                        freeResult = true;
                }
            }
            else
            {
                //  Release the facility item, and if R is not specified, release the associated name items also.
                FacilityItem::IDENTIFIER facItemId = pFacilityItem->getIdentifier();
                freeResult = freeReleaseFacilityItem( context, options, pFacilityItem );
                if ( (options & OPTB_R) == 0 )
                    freeReleaseNameItems( context, facItemId );
                else
                    freeUnresolveNameItems( context, facItemId );
            }
        }
    }

    //  Done
    if ( freeResult )
        result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_COMPLETE, "FREE" ) );

    pRunInfo->detach();
    return result;
}


//  qual()
//
//  Invoked directly by various Exec objects (or maybe not), and indirectly via CSI for @QUAL statement.
//  We validate options, and the directoryId and qualifier fields of specification.
//  If other fields of specification happen to contain stuff, we ignore them.
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo which is to be affected (might not be the RunInfo associated with the Activity)
//      options:            options given on the @QUAL
//      qualifier:          if non-empty, this is the qualifier given on the @QUAL command
FacilitiesManager::Result
FacilitiesManager::qual
(
    RunInfo* const              pRunInfo,
    const UINT32                options,
    const std::string&          qualifier
) const
{
    Result result;

    //  Check for proper options.  We allow none, or D, or R (but not D and R).
    checkOptions( options, OPTB_D | OPTB_R, &result );

    bool dOpt = options & OPTB_D ? true : false;
    bool rOpt = options & OPTB_R ? true : false;
    if ( dOpt && rOpt )
    {
        result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_ILLEGAL_OPTION_COMBINATION, "D", "R" ) );
        return result;
    }

    //  Make sure we have a qualifier for no options or D option;
    //  Make sure we have none for r option.
    if ( !rOpt && (qualifier.size() == 0) )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_DIRECTORY_ID_OR_QUALIFIER_REQUIRED );
        return result;
    }

    if ( rOpt && (qualifier.size() != 0) )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_DIRECTORY_ID_AND_QUALIFIER_NOT_ALLOWED );
        return result;
    }

    //  If we found an error anywhere, don't do anything, but do set up the status bit mask and contingency info.
    //  Control mode won't use the status bit mask, since it uses the facility codes;
    //  Program mode won't use it either, since it will raise the contingency instead...
    //  But we set it up anyway - maybe it will be useful for debugging somewhere.
    if ( result.m_StatusCodeInstances.size() > 0 )
    {
        result.m_StatusBitMask.setW( 0600000000000 );   //  General CSF error
        result.m_ContingencyType = 012;                 //  ERR mode
        result.m_ErrorType = 04;                        //  ER / misc error
        result.m_ErrorCode = 040;                       //  Syntax error on CSF image
        return result;
    }

    //  We're good to go.  Attach the RunInfo object to lock it, update it, and release it
    pRunInfo->attach();

    if ( rOpt )
    {
        pRunInfo->setDefaultQualifier( pRunInfo->getProjectId() );
        pRunInfo->setImpliedQualifier( pRunInfo->getProjectId() );
    }
    else if ( dOpt )
    {
        pRunInfo->setDefaultQualifier( qualifier );
    }
    else
    {
        pRunInfo->setImpliedQualifier( qualifier );
    }

    result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_COMPLETE, "QUAL" ) );
    pRunInfo->detach();
    return result;
}


//  releaseFacilityItemRunTermination()
//
//  A special version of @FREE which is invoked by Exec at run termination.
//  If anything goes wrong, we handle it (Exec doesn't want to know about it).
//  We do nothing with NameItems; those are going away anyhow.
//
//  Parameters:
//      pActivity:          Exec activity which is calling us
//      pSecurityContext:   Exec's security context
//      pRunInfo:           pointer to RunInfo of the run which is terminating
//      identifier:         uniquely identifies a particular FACITEM in the RunInfo
//      errorTermFlag:      true if the run is terminating in error
void
FacilitiesManager::releaseFacilityItemRunTermination
(
    Activity* const                 pActivity,
    SecurityContext* const          pSecurityContext,
    RunInfo* const                  pRunInfo,
    const FacilityItem::IDENTIFIER  identifier,
    const bool                      errorTermFlag
) const
{
    FacilityItem* pFacItem = pRunInfo->getFacilityItem( identifier );

    //  Put together a Context object
    Result result;
    Context context( pActivity, pSecurityContext, pRunInfo, &result );

    //  Call the appropriate routine based on the fac item type
    NonStandardFacilityItem* pnsfi = dynamic_cast<NonStandardFacilityItem*>( pFacItem );
    DiskFacilityItem* pdfi = dynamic_cast<DiskFacilityItem*>( pFacItem );
    TapeFacilityItem* ptfi = dynamic_cast<TapeFacilityItem*>( pFacItem );
    if ( pnsfi )
    {
        //  TODO:ABSASG
    }
    else if ( pdfi )
    {
        //  Delete file?  (K option --> yes, D option --> if !errorTermFlag)
        //  *or* (C option --> yes if errorTermFlag)...
        //  Note that there are other reasons why the file might be deleted at this point;
        //  lower-level code notes that and makes it happen.
        bool deleteFlag = pdfi->getDeleteOnAnyTermFlag()
                            || (!errorTermFlag && pdfi->getDeleteOnNormalTermFlag())
                            || (errorTermFlag && pdfi->getCatalogOnNormalTermFlag());
        freeReleaseDiskFile( context, pdfi, deleteFlag );
    }
    else if ( ptfi )
    {
        //  Delete file?  (K option --> yes, D option --> if !errorTermFlag)
        bool deleteFlag = ptfi->getDeleteOnAnyTermFlag()
                            || (!errorTermFlag && ptfi->getDeleteOnNormalTermFlag());
        freeReleaseTapeFile( context, ptfi, deleteFlag );
    }
    else
    {
        std::string logMsg = "FacilitiesManager::releaseFacilityItem():Undeterminable fac item type for ";
        logMsg += pFacItem->getQualifier() + "*" + pFacItem->getFileName();
        SystemLog::write( logMsg );
        m_pExec->stopExec( Exec::SC_FACILITIES_INVENTORY );
    }
}


//  shutdown()
//
//  ExecManager interface
void
FacilitiesManager::shutdown()
{
    SystemLog::write( "FacilitiesManager::shutdown()" );
}


//  startup()
//
//  ExecManager interface
bool
FacilitiesManager::startup()
{
    SystemLog::write( "FacilitiesManager::startup()" );

    //  Reload EXEC config values
    m_DefaultEquipTape = m_pExec->getConfiguration().getStringValue( "TDFALT" );    // for @ASG of tape file with no eqp type spec'd
    m_DefaultEquipMassStorage = m_pExec->getConfiguration().getStringValue( "MDFALT" );
    m_DefaultEquipWordAddressable = m_pExec->getConfiguration().getStringValue( "WDFALT" );
    m_DefaultMaxGranules = static_cast<COUNT32>(m_pExec->getConfiguration().getIntegerValue( "MAXGRN" ));
    m_ReleaseUnusedReserve = m_pExec->getConfiguration().getBoolValue( "RELUNUSEDRES" );
    m_PrivateByAccount = m_pExec->getConfiguration().getBoolValue( "SSPBP" );
    m_RejectZOptionOnBatch = m_pExec->getConfiguration().getBoolValue( "ZOPTBATCHREJ" );

    //  More initialization
    m_pDefaultEquipTape = findEquipmentType( m_DefaultEquipTape );
    m_pDefaultEquipMassStorage = findEquipmentType( m_DefaultEquipMassStorage );
    m_pDefaultEquipWordAddressable = findEquipmentType( m_DefaultEquipWordAddressable );
    if ( !m_pDefaultEquipTape || !m_pDefaultEquipMassStorage || !m_pDefaultEquipWordAddressable )
    {
        SystemLog::write( "FacilitiesManager::Cannot find default equipment type" );
        return false;
    }

    return true;
}


//  terminate()
//
//  ExecManager interface
void
FacilitiesManager::terminate()
{
    SystemLog::write( "FacilitiesManager::terminate()" );
}


//  use()
//
//  Invoked directly by various Exec objects (or maybe not), and indirectly via CSI for @USE statement.
//
//  Our implementation differs perhaps from the canonical behavior of the EXEC.
//  The ECL guide states explicitly that name-2 must refer either to a previously-specified internal name,
//  or to an external file.  A subsequent example breaks that rule by showing name-2 referring to an
//  internal name which is specified in the next step (twice)... we are going to ignore the example and
//  follow the explicit definition of name-2.
//
//  Note that we do not make an explicit connection to the external file at the time of the @USE statement.
//  This means that a NameItem may reference a putative file which does not, in fact, exist.  We only
//  resolve the association of a NameItem's external reference to an actual file, at the time of an @ASG
//  or @FREE statement employing the internal name.
//
//  We can therefore establish that a RunInfo::NameItem refers only to an external file, and that if a
//  user supplies an internal name for name-2, that we will find the associated NameItem, and copy its
//  external file specification to the new NameItem we are creating.
//
//  Parameters:
//      pActivity:          pointer to requesting Activity object
//      pRunInfo:           pointer to RunInfo which is to be affected (might not be the RunInfo associated with the Activity)
//      options:            options given on the @CAT
//      specification1:     the FileSpecification representing the internal file name to be assigned
//      specification2:     the FileSpecification representing the target
FacilitiesManager::Result
FacilitiesManager::use
(
    RunInfo* const              pRunInfo,
    const UINT32                options,
    const FileSpecification&    specification1,
    const FileSpecification&    specification2
) const
{
    Result result;

    {//TODO:DEBUG
        std::cout << "FacilitiesManager::use() " << specification1 << "  " << specification2 << std::endl;
    }

    //  Check for proper options.  We allow only I, as an option.
    if ( (options != 0) && (options != OPTB_I) )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_I_OPTION_ONLY_ON_USE );
        result.m_StatusBitMask.logicalOr( 0400000400000 );
        return result;
    }

    bool iOpt = options & OPTB_I ? true : false;

    //  Ensure that filespec1 is an internal name (no qual, cycle, etc)
    if ( !specification1.isFileNameOnly() )
    {
        result.m_StatusCodeInstances.push_back( FSCODE_INTERNAL_NAME_IS_REQUIRED );
        result.m_StatusBitMask.logicalOr( 0600000000000 );
        return result;
    }

    pRunInfo->attach();

    //  If filespec2 is potentially an internal name, try to resolve spec2 to an existing name-item.
    //  If that name item exists, and spec1 and spec2 are equivalent, do nothing.
    //  If they are NOT equivalent, create a new NameItem for the given filespec1, with external linkage
    //  matching the found name item (effectively replacing the given filespec2 with the NameItem's spec).
    bool match = false;
    if ( specification2.isFileNameOnly() )
    {
        RunInfo::NameItem* pNameItem = pRunInfo->getNameItem( specification2.m_FileName );
        if ( pNameItem != 0 )
        {
            //  Found a NameItem matching spec2 - if spec1's are not identical, create a new NameItem
            //  and copy the filespec (and facitem identifier) from the existing name item to the new one.
            if ( specification1.m_FileName.compareNoCase( pNameItem->m_Name ) != 0 )
            {
                RunInfo::NameItem* pNewItem = new RunInfo::NameItem( specification1.m_FileName, pNameItem->m_FileSpecification, iOpt );
                pNewItem->m_FacilityItemIdentifier = pNameItem->m_FacilityItemIdentifier;
                pRunInfo->establishNameItem( specification1.m_FileName, pNewItem );
            }
            match = true;
        }
    }

    //  If we didn't set up a link to a NameItem yet, do so now.
    if ( !match )
    {
        RunInfo::NameItem* pNameItem = new RunInfo::NameItem( specification1.m_FileName, specification2, iOpt );
        pRunInfo->establishNameItem( specification1.m_FileName, pNameItem );

        //  Is the referenced external file already assigned to the run?
        CITFACITEMS itfi;
        if ( isFileAssigned( pNameItem->m_FileSpecification, *pRunInfo, &itfi ) )
            pNameItem->m_FacilityItemIdentifier = itfi->first;
    }

    result.m_StatusCodeInstances.push_back( StatusCodeInstance( FSCODE_COMPLETE, "USE" ) );
    pRunInfo->detach();
    return result;
}


//  Result::clear()
//
//  Clears all the fields back to the initial state
void
FacilitiesManager::Result::clear()
{
    m_StatusCodeInstances.clear();
    m_FileIndex[0].setW( 0 );
    m_FileIndex[1].setW( 0 );
    m_ContingencyType = 0;
    m_ErrorType = 0;
    m_ErrorCode = 0;
}


//  Result::containsErrorStatusCode()
//
//  A convenient method for determining whether the status code container for a Result object
//  contains any error status code.
bool
FacilitiesManager::Result::containsErrorStatusCode() const
{
    for ( const StatusCodeInstance& inst : m_StatusCodeInstances )
    {
        const StatusCodeInfo* pInfo = FacilitiesManager::findStatusCodeInfo( inst.m_StatusCode );
        if ( (pInfo != 0) && (pInfo->m_StatusCodeCategory == FSSCAT_ERROR) )
            return true;
    }

    return false;
}


//  Result::containsHoldStatusCode()
//
//  A convenient method for determining whether the status code container for a Result object
//  contains any error status code.
bool
FacilitiesManager::Result::containsHoldStatusCode() const
{
    for ( const StatusCodeInstance& inst : m_StatusCodeInstances )
    {
        const StatusCodeInfo* pInfo = FacilitiesManager::findStatusCodeInfo( inst.m_StatusCode );
        if ( pInfo && pInfo->m_HoldCondition )
            return true;
    }

    return false;
}


//  Result::containsStatusCode()
//
//  A convenient method for determining whether the status code container for a Result object
//  contains a particular status code.
bool
FacilitiesManager::Result::containsStatusCode
(
    const StatusCode        statusCode
) const
{
    for ( const StatusCodeInstance& inst : m_StatusCodeInstances )
    {
        if ( inst.m_StatusCode == statusCode )
            return true;
    }

    return false;
}


//  Result::convertHoldStatusCodes()
//
//  Converts all FSCODE_HELD status code instances to corresponding FSCODE_HOLD_xxx_REJECTED_Z instances.
void
FacilitiesManager::Result::convertHoldStatusCodes()
{
    for ( INDEX sx = 0; sx < m_StatusCodeInstances.size(); ++sx )
    {
        const StatusCodeInfo* const psci = findStatusCodeInfo( m_StatusCodeInstances[sx].m_StatusCode );
        if ( psci && psci->m_HoldCondition )
            m_StatusCodeInstances[sx] = StatusCodeInstance( psci->m_ReplacementCode );
    }
}


//  Result::postToSystemLog()
//
//  Posts facilities results to the system log
void
FacilitiesManager::Result::postToSystemLog() const
{
    for ( const StatusCodeInstance& inst : m_StatusCodeInstances )
    {
        SystemLog::getInstance()->write( inst.getString() );
    }
}


//  Result::postToPrint()
//
//  Posts facilities results to PRINT$
void
FacilitiesManager::Result::postToPrint
(
    ControlModeRunInfo* const   pRunInfo
) const
{
    pRunInfo->attach();
    for ( const StatusCodeInstance& inst : m_StatusCodeInstances )
        pRunInfo->postToPrint( inst.getString() );
    pRunInfo->detach();
}


//  StatusCodeInstance::getString()
//
//  Constructs a string to be displayed upon PRINT$
std::string
FacilitiesManager::StatusCodeInstance::getString() const
{
    const StatusCodeInfo* pInfo = FacilitiesManager::findStatusCodeInfo( m_StatusCode );
    if ( pInfo == 0 )
        return "";

    //  Construct I, W, or E
    std::stringstream strm;
    if ( pInfo->m_StatusCodeCategory == FSSCAT_INFO )
        strm << "I:";
    else if ( pInfo->m_StatusCodeCategory == FSSCAT_WARNING )
        strm << "W:";
    else
        strm << "E:";

    //  Construct output of octal code value
    strm << std::oct << std::setw( 6 ) << std::setfill( '0' ) << static_cast<unsigned int>( m_StatusCode );
    strm << " ";

    //  Construct message with possible variable fields filled in
    INDEX textLimit = strlen( pInfo->m_pText );
    INDEX vx = 0;
    for ( INDEX cx = 0; cx < textLimit; ++cx )
    {
        char ch = pInfo->m_pText[cx];
        if ( ch != '%' )
            strm << ch;
        else if ( vx < m_VariableInfoStrings.size() )
            strm << m_VariableInfoStrings[vx++];
    }

    return strm.str();
}



//  public statics

std::ostream&
operator<<
(
    std::ostream&                       stream,
    const FacilitiesManager::Field&     field
)
{
    for ( INDEX sfx = 0; sfx < field.size(); ++sfx )
    {
        if ( sfx )   stream << "/";
        stream << field[sfx];
    }
    return stream;
}


std::ostream&
operator<<
(
    std::ostream&                       stream,
    const FacilitiesManager::FieldList& fieldList
)
{
    for ( INDEX fx = 0; fx < fieldList.size(); ++fx )
    {
        if ( fx )   stream << ",";
        stream << fieldList[fx];
    }
    return stream;
}

