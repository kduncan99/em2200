//  FacilitiesManager class
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EXECLIB_FACILITIES_MANAGER_H
#define     EXECLIB_FACILITIES_MANAGER_H



#include    "Activity.h"
#include    "ControlModeRunInfo.h"
#include    "DiskFacilityItem.h"
#include    "ExecManager.h"
#include    "FacilityItem.h"
#include    "FileSpecification.h"
#include    "MFDManager.h"
#include    "NonStandardFacilityItem.h"
#include    "RunInfo.h"
#include    "SecurityContext.h"
#include    "TapeFacilityItem.h"



class   FacilitiesManager : public ExecManager
{
public:
    enum    StatusCode
    {
        FSCODE_NONE                                             = 0,
        FSCODE_HELD_FOR_UNIT_FOR_ABS_DEVICE                     = 0000133,
        FSCODE_HELD_FOR_UNIT_FOR_ABS_REM_DISK                   = 0000233,
        FSCODE_HELD_FOR_PACK                                    = 0000333,
        FSCODE_HELD_FOR_REEL                                    = 0000433,
        FSCODE_HELD_FOR_COM_LINE                                = 0000533,
        FSCODE_HELD_FOR_COM_GROUP                               = 0000633,
        FSCODE_HELD_FOR_MASS_STORAGE_SPACE                      = 0000733,
        FSCODE_HELD_FOR_TAPE_UNIT                               = 0001033,
        FSCODE_HELD_FOR_EXCLUSIVE_FILE_USE_RELEASE              = 0001133,
        FSCODE_HELD_FOR_NEED_OF_EXCLUSIVE_USE                   = 0001233,
        FSCODE_HELD_FOR_DISK_UNIT                               = 0001333,
        FSCODE_HELD_FOR_ROLLBACK                                = 0001433,
        FSCODE_HELD_FOR_FILE_CYCLE_CONFLICT                     = 0001533,
        FSCODE_HELD_FOR_DISK_PACK                               = 0001633,
        FSCODE_DEVICE_IS_SELECTED                               = 0001733,
        FSCODE_HELD_FOR_CONTROL_OF_CACHING                      = 0002033,
        FSCODE_HELD_FOR_DISKETTE_UNIT                           = 0002133,
        FSCODE_HELD_FOR_DISKETTE                                = 0002233,
        FSCODE_COMPLETE                                         = 0002333,
        FSCODE_FILE_IS_ALREADY_ASSIGNED                         = 0120133,
        FSCODE_FILE_IS_ALREADY_ASSIGNED_THIS_IMAGE_IGNORED      = 0120233,
        FSCODE_FILE_IS_ASSIGNED_TO_ANOTHER_RUN                  = 0120333,
        FSCODE_FILE_NAME_NOT_KNOWN_TO_THIS_RUN                  = 0120433,
        FSCODE_FILE_NAME_NOT_UNIQUE                             = 0120533,
        FSCODE_MFD_IO_ERROR_DURING_FREE                         = 0120633,
//W:120733 Line is assigned in simulation mode.
//W:121033 Not all warning messages printed.
        FSCODE_PLACEMENT_FIELD_IGNORED                          = 0121133,
        FSCODE_FILE_ON_PRINT_QUEUE                              = 0121233,
        FSCODE_READ_KEY_EXISTS                                  = 0121333,
        FSCODE_CATALOGED_AS_READ_ONLY                           = 0121433,
        FSCODE_FILE_WAS_ASSIGNED_DURING_SYSTEM_FAILURE          = 0122133,
//W:122233 File is unloaded.
        FSCODE_WRITE_KEY_EXISTS                                 = 0122333,
        FSCODE_CATALOGED_AS_WRITE_ONLY                          = 0122433,
        FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_IQXYZ      = 0122533,
        FSCODE_OPTION_CONFLICT_WITH_PREVIOUS_OPTIONS_MISC       = 0122633,
//W:122733 Expiration exceeds the configured maximum. Configured maximum used.
//W:123033 Read/write keys are ignored in an ownership system for owned files.
//W:123133 G option not enforced.
        FSCODE_ALREADY_EXCLUSIVELY_ASSIGNED                     = 0123233,
        FSCODE_SYSTEM_CONFIGURED_TO_IGNORE_Z_OPTION_IN_BATCH    = 0123333,
        FSCODE_SYSTEM_CONFIGURED_TO_RELEASE_INITIAL_RESERVE     = 0123433,
//W:123533 Security compartments allow read only access.
//W:123633 The file cycle set is private and owned-therefore this cycle is semi-private.
//W:123733 The file cycle set is semi-private and owned-therefore this cycle set is semi-private
//W:124033 The file cycle set is public and owned-therefore this cycle is public
//W:124133 Security does not allow the use of unlabeled tapes. If written to, this tape will become labeled.
//W:124233 The file is security disabled.
//W:124333 Media Manager is not available.
//W:124433 File cannot be deleted because an XPC contains data for this file.
//W:124733 The memory assign mnemonic specification is ignored for an XPC cached file.
        FSCODE_ASSIGN_MNEMONIC_IS_NOT_CONFIGURED                = 0201033,
        FSCODE_ILLEGAL_OPTION_COMBINATION                       = 0201433,
        FSCODE_ILLEGAL_OPTION                                   = 0201533,
        FSCODE_ASSIGN_MNEMONIC_DOES_NOT_SUPPORT_6_BIT_PACKED    = 0204433,
        FSCODE_MNEMONIC_DOES_NOT_ALLOW_NOISE_FIELD              = 0204633,
        FSCODE_ASSIGN_MNEMONIC_TOO_LONG                         = 0204733,
        FSCODE_ASSIGN_MNEMONIC_MUST_BE_WORD_ADDRESSABLE         = 0241133,
        FSCODE_ILLEGAL_ATTEMPT_TO_CHANGE_ASSIGNMENT_TYPE        = 0241533,
        FSCODE_ATTEMPT_TO_CHANGE_GENERIC_TYPE                   = 0241633,
        FSCODE_ATTEMPT_TO_CHANGE_GRANULARITY                    = 0241733,
        FSCODE_ATTEMPT_TO_CHANGE_INITIAL_RESERVE                = 0242033,
        FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_DIFFERENT_ACCOUNT = 0242133,
        FSCODE_ATTEMPT_TO_CHANGE_MAX_GRANULES_WRITE_INHIBITED   = 0242233,
        FSCODE_ILLEGAL_VALUE_FOR_FCYCLE                         = 0242433,
        FSCODE_FILE_CYCLE_OUT_OF_RANGE                          = 0242533,
        FSCODE_ILLEGAL_DROPPING_OF_PRIVATE_FILE                 = 0243233,
        FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_FORMAT               = 0243333,
        FSCODE_CSI_SYNTAX_ERROR                                 = 0243533,
        FSCODE_CSI_BAD_IMAGE_LENGTH                             = 0243633,
        FSCODE_CSI_ILLEGAL_CONTROL_STATEMENT_TYPE               = 0243733,
        FSCODE_FILENAME_IS_REQUIRED                             = 0244333,
        FSCODE_FILE_IS_ALREADY_CATALOGED                        = 0244433,
        FSCODE_FILE_IS_NOT_CATALOGED                            = 0244533,
        FSCODE_FILE_CANNOT_BE_RECOVERED_MFD_CORRUPTED           = 0244633,
//E:245033 Facility inventory internal error 1, undefined generic equipment type.
        FSCODE_ATTEMPT_TO_DELETE_VIA_FREE                       = 0245133,
        FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_GRANULARITY          = 0245233,
        FSCODE_INSUFFICIENT_NUMBER_OF_UNITS_AVAILABLE           = 0245433,
        FSCODE_INTERNAL_NAME_IS_REQUIRED                        = 0245533,
        FSCODE_IO_ERROR_ENCOUNTERED_ON_MFD                      = 0245733,
        FSCODE_FILE_INITIAL_RESERVE_EXCEEDED                    = 0246233,
        FSCODE_ILLEGAL_VALUE_FOR_INITIAL_RESERVE                = 0246333,
        FSCODE_READ_AND_OR_WRITE_KEYS_NEEDED                    = 0246433,
        FSCODE_ILLEGAL_VALUE_SPECIFIED_FOR_LOGICAL_CHANNEL      = 0247033,
        FSCODE_MAX_GRANULES_LESS_THAN_HIGHEST_ALLOCATED         = 0247133,
        FSCODE_FILE_MAX_GRANULES_EXCEEDED                       = 0247233,
        FSCODE_ILLEGAL_VALUE_FOR_MAXIMUM                        = 0247333,
        FSCODE_MAXIMUM_IS_LESS_THAN_RESERVE                     = 0247433,
        FSCODE_MASS_STORAGE_OVERFLOW                            = 0250133,
        FSCODE_ILLEGAL_VALUE_FOR_NUMBER_OF_UNITS                = 0250333,
        FSCODE_ILLEGAL_NOISE_CONSTANT_VALUE                     = 0250433,
        FSCODE_ILLEGAL_USE_OF_I_OPTION                          = 0250633,
        FSCODE_I_OPTION_ONLY_ON_USE                             = 0250733,
        FSCODE_FCYCLE_PLUS_ONE_ILLEGAL_WITH_A                   = 0252033,
        FSCODE_INCORRECT_PRIVACY_KEY                            = 0252533,
        FSCODE_INCORRECT_READ_KEY                               = 0253333,
        FSCODE_FILE_NOT_CATALOGED_WITH_READ_KEY                 = 0253433,
        FSCODE_RELATIVE_FCYCLE_CONFLICT                         = 0253733,
        FSCODE_FILE_BACKUP_IS_NOT_AVAILABLE                     = 0254333,
        FSCODE_IMAGE_CONTAINS_UNDEFINED_FIELD_OR_SUBFIELD       = 0255733,
        FSCODE_UNITS_AND_LOGICAL_SUBFIELDS_NOT_ALLOWED_FOR_CAT  = 0256333,
        FSCODE_ATTEMPT_TO_CHANGE_TO_WORD_ADDRESSABLE_NOT_ALLOWED= 0256533,
        FSCODE_INCORRECT_WRITE_KEY                              = 0256633,
        FSCODE_FILE_NOT_CATALOGED_WITH_WRITE_KEY                = 0256733,
        FSCODE_HOLD_FOR_REEL_REJECTED_Z                         = 0257033,
        FSCODE_HOLD_FOR_PACK_REJECTED_Z                         = 0257133,
        FSCODE_HOLD_FOR_TAPE_UNIT_REJECTED_Z                    = 0257233,
        FSCODE_HOLD_FOR_DISK_UNIT_REJECTED_Z                    = 0257333,
        FSCODE_HOLD_FOR_X_USE_REJECTED_Z                        = 0257433,
        FSCODE_HOLD_FOR_RELEASE_OF_X_USE_REJECTED_Z             = 0257533,
        FSCODE_HOLD_FOR_ROLLBACK_REJECTED_Z                     = 0257633,
        FSCODE_HOLD_FOR_COM_LINE_REJECTED_Z                     = 0257733,
        FSCODE_HOLD_FOR_COM_GROUP_REJECTED_Z                    = 0260033,
        FSCODE_HOLD_FOR_REM_DISK_REJECTED_Z                     = 0260133,
        FSCODE_HOLD_FOR_DEV_CU_REJECTED_Z                       = 0260233,
        FSCODE_HOLD_FOR_FCYCLE_CONFLICT_REJECTED_Z              = 0260333,
        FSCODE_HOLD_FOR_MASS_STORAGE_SPACE_REJECTED_Z           = 0260433,
        FSCODE_RUN_ABORTED                                      = 0260733,
        FSCODE_HOLD_FOR_CONTROL_OF_CACHING_REJECTED_Z           = 0261433,
        FSCODE_HOLD_FOR_DISKETTE_UNIT_REJECTED_Z                = 0261733,
        FSCODE_DIRECTORY_ID_AND_QUALIFIER_NOT_ALLOWED           = 0262633,
        FSCODE_DIRECTORY_ID_OR_QUALIFIER_REQUIRED               = 0263033,
        FSCODE_NO_TAPE_UNITS_CONFIGURED                         = 0264133,
        FSCODE_HOLD_FOR_CTL_REJECTED_Z                          = 0266333,
        FSCODE_HOLD_FOR_NETWORK_DEVICE_REJECTED_Z               = 0270033,
    };


    enum    StatusCodeCategory
    {
        FSSCAT_INFO,
        FSSCAT_WARNING,
        FSSCAT_ERROR,
    };


    struct  StatusCodeInfo
    {
    public:
        const StatusCode            m_StatusCode;
        const StatusCodeCategory    m_StatusCodeCategory;
        const char* const           m_pText;
        const bool                  m_HoldCondition;
        const StatusCode            m_ReplacementCode;      //  if non-zero, this object represents a hold condition,
                                                            //      and this value is the alternative non-hold status code
                                                            //      which corresponds to the underlying condition.
    };

    struct  StatusCodeInstance
    {
        StatusCode                  m_StatusCode;
        VSTRING                     m_VariableInfoStrings;

        StatusCodeInstance( const StatusCode        statusCode,
                            const std::string&      variableInfoString1 = "",
                            const std::string&      variableInfoString2 = "" )
            :m_StatusCode( statusCode )
        {
            if ( variableInfoString1.size() > 0 )
            {
                m_VariableInfoStrings.push_back( variableInfoString1 );
                if ( variableInfoString2.size() > 0 )
                    m_VariableInfoStrings.push_back( variableInfoString2 );
            }
        }

        std::string         getString() const;
    };

    class   Result
    {
    public:
        std::vector<StatusCodeInstance> m_StatusCodeInstances;
        Word36                          m_StatusBitMask;
        Word36                          m_FileIndex[2];
        UINT8                           m_ContingencyType;
        UINT8                           m_ErrorType;
        UINT8                           m_ErrorCode;

        Result()
            :m_ContingencyType( 0 ),
            m_ErrorType( 0 ),
            m_ErrorCode( 0 )
        {}

        void                    clear();
        bool                    containsErrorStatusCode() const;
        bool                    containsHoldStatusCode() const;
        bool                    containsStatusCode( const StatusCode statusCode ) const;
        void                    convertHoldStatusCodes();
        void                    postToSystemLog() const;
        void                    postToPrint( ControlModeRunInfo* const pRunInfo ) const;
    };


    class   SubField : public std::string
    {
    public:
        SubField( const std::string& value )
            :std::string( value )
        {}
    };

    class   Field : public std::vector<SubField> {};

    class   FieldList : public std::vector<Field> {};

private:
    enum    EquipTypeGroup
    {
        FACETG_SECTOR_DISK,
        FACETG_WORD_DISK,
        FACETG_TAPE,
    };

    //  Contains info for changing MFD and FacItem information for a file which is already assigned
    class   AlreadyAssignedChangesDisk
    {
    public:
        UINT32                  m_NewInitialReserve;
        UINT32                  m_NewMaxGranules;
        bool                    m_UpdateInitialReserve;
        bool                    m_UpdateMaxGranules;
        bool                    m_SetExclusiveUse;
        bool                    m_SetReleaseOnTaskEnd;

        AlreadyAssignedChangesDisk()
            :m_NewInitialReserve( 0 ),
            m_NewMaxGranules( 0 ),
            m_UpdateInitialReserve( false ),
            m_UpdateMaxGranules( false ),
            m_SetExclusiveUse( false ),
            m_SetReleaseOnTaskEnd( false )
        {}
    };

    class   Context
    {
    public:
        Activity* const         m_pActivity;            //  pointer to requesting activity
        SecurityContext* const  m_pSecurityContext;     //  pointer to security context for request
        RunInfo* const          m_pRunInfo;             //  pointer to RunInfo to be affected
        Result* const           m_pResult;              //  pointer to Result object

        Context( Activity* const        pActivity,
                 SecurityContext* const pSecurityContext,
                 RunInfo* const         pRunInfo,
                 Result* const          pResult )
                 :m_pActivity( pActivity ),
                 m_pSecurityContext( pSecurityContext ),
                 m_pRunInfo( pRunInfo ),
                 m_pResult( pResult )
        {}
    };

    //  Defines an assign mnemonic, which then drives which models are chosen for an assignment
    class   EquipmentType
    {
    public:
        const char* const                   m_pMnemonic;
        const EquipmentCategory             m_EquipmentCategory;    //  very general categorization
        const EquipTypeGroup                m_EquipmentTypeGroup;   //  tape, word-addr, or sector-addr
        std::list<const EquipmentModel*>    m_EquipmentModels;      //  pointers to all the EM's which will satisfy this mnemonic

        EquipmentType( const char* const            pMnemonic,
                        const EquipmentCategory     category,
                        const EquipTypeGroup        group,
                        const COUNT                 modelTableSize = 0,
                        const EquipmentModel* const modelTable[] = 0 )
            :m_pMnemonic( pMnemonic ),
            m_EquipmentCategory( category ),
            m_EquipmentTypeGroup( group )
        {
            for ( INDEX mx = 0; mx < modelTableSize; ++mx )
                m_EquipmentModels.push_back( modelTable[mx] );
        }
    };

    //  Contains info for changing MFD information for a cataloged file which is newly-assigned
    class   NewlyAssignedChangesDisk
    {
    public:
        UINT32                  m_NewInitialReserve;
        UINT32                  m_NewMaxGranules;
        bool                    m_UpdateInitialReserve;
        bool                    m_UpdateMaxGranules;

        NewlyAssignedChangesDisk()
            :m_NewInitialReserve( 0 ),
            m_NewMaxGranules( 0 ),
            m_UpdateInitialReserve( false ),
            m_UpdateMaxGranules( false )
        {}
    };

    class   DiskSubfieldInfo
    {
    public:
        const EquipmentType*    m_pEquipmentType;
        MassStorageGranularity  m_Granularity;
        bool                    m_GranularitySpecified;
        UINT32                  m_InitialReserve;
        bool                    m_InitialReserveSpecified;
        UINT32                  m_MaxGranules;
        bool                    m_MaxGranulesSpecified;

        DiskSubfieldInfo()
            :m_pEquipmentType( 0 ),
            m_Granularity( MSGRAN_TRACK ),
            m_GranularitySpecified( false ),
            m_InitialReserve( 0 ),
            m_InitialReserveSpecified( false ),
            m_MaxGranules( 0 ),
            m_MaxGranulesSpecified( false )
        {}
    };

    class   TapeSubfieldInfo
    {
    public:
        const EquipmentType*    m_pEquipmentType;

        TapeSubfieldInfo()
            :m_pEquipmentType( 0 )
        {}
    };


    std::string                 m_DefaultEquipTape;             //  from TDFALT
    static const EquipmentType* m_pDefaultEquipTape;
    std::string                 m_DefaultEquipMassStorage;      //  from MDFALT
    static const EquipmentType* m_pDefaultEquipMassStorage;
    std::string                 m_DefaultEquipWordAddressable;  //  from WDFALT
    static const EquipmentType* m_pDefaultEquipWordAddressable;
    COUNT32                     m_DefaultMaxGranules;           //  from MAXGRN
    MFDManager* const           m_pMFDManager;
    bool                        m_ReleaseUnusedReserve;         //  from RELUNUSEDRES
    bool                        m_PrivateByAccount;             //  from SSPBP
    bool                        m_RejectZOptionOnBatch;         //  from ZOPTBATCHREJ

    std::map<SuperString, EquipmentModel*>          m_EquipmentModels;
    std::vector<EquipmentType*>                     m_EquipmentTypes;

    static StatusCodeInfo                           m_StatusCodeInfoTable[];
    static COUNT                                    m_StatusCodeInfoTableSize;

    //  private methods
    bool                    asgAllocateInitialReserve( const Context&           context,
                                                       DiskFacilityItem* const  pDiskItem ) const;
    bool                    asgAlreadyAssignedDiskExec( const Context&                               context,
                                                        const FileSpecification&                     fileSpecification,
                                                        const CITFACITEMS                            itFacItem,
                                                        const MFDManager::FileSetInfo&               fileSetInfo,
                                                        const INDEX                                  fileEntryIndex,
                                                        const MFDManager::MassStorageFileCycleInfo&  fileCycleInfo,
                                                        const AlreadyAssignedChangesDisk&            changes ) const;
    bool                    asgCatalog( const Context&              context,
                                        const UINT32                options,
                                        const FileSpecification&    fileSpecification,
                                        const FieldList&            additionalFields ) const;
    bool                    asgCatalogAlreadyAssigned( const Context&           context,
                                                       const UINT32             options,
                                                       const FileSpecification& fileSpecification,
                                                       const FieldList&         additionalFields,
                                                       const CITFACITEMS&       itFacItem ) const;
    bool                    asgCatalogAlreadyAssignedDisk( const Context&                       context,
                                                           const UINT32                         options,
                                                           const FileSpecification&             fileSpecification,
                                                           const FieldList&                     additionalFields,
                                                           const CITFACITEMS&                   itFacItem ) const;
    bool                    asgCatalogAlreadyAssignedDiskCheck( const Context&                      context,
                                                                const UINT32                        options,
                                                                const FieldList&                    additionalFields,
                                                                const StandardFacilityItem* const   pFacItem,
                                                                AlreadyAssignedChangesDisk* const   pChanges ) const;
    bool                    asgCatalogAlreadyAssignedTape( const Context&           context,
                                                           const UINT32             options,
                                                           const FileSpecification& fileSpecification,
                                                           const FieldList&         additionalFields,
                                                           const CITFACITEMS&       itFacItem ) const;
    bool                    asgCatalogNewDisk( const Context&                   context,
                                               const UINT32                     options,
                                               const FieldList&                 additionalFields,
                                               const FileSpecification&         fileSpecification,
                                               const EquipmentType*             pEquipmentType,
                                               const MFDManager::FileSetInfo*   pFileSetInfo ) const;
    bool                    asgCatalogNewDiskCycle( const Context&                  context,
                                                    const UINT32                    options,
                                                    const FieldList&                additionalFields,
                                                    const FileSpecification&        fileSpecification,
                                                    const EquipmentType*            pEquipmentType,
                                                    const MFDManager::FileSetInfo*  pFileSetInfo ) const;
    bool                    asgCatalogNewDiskSetCycle( const Context&           context,
                                                       const UINT32             options,
                                                       const FieldList&         additionalFields,
                                                       const FileSpecification& fileSpecification,
                                                       const EquipmentType*     pEquipmentType ) const;
    bool                    asgCatalogNewTape( const Context&                   context,
                                               const UINT32                     options,
                                               const FieldList&                 additionalFields,
                                               const FileSpecification&         fileSpecification,
                                               const EquipmentType*             pEquipmentType,
                                               const MFDManager::FileSetInfo*   pFileSetInfo ) const;
    bool                    asgCatalogNewTapeCycle( const Context&                  context,
                                                    const UINT32                    options,
                                                    const FieldList&                additionalFields,
                                                    const FileSpecification&        fileSpecification,
                                                    const EquipmentType*            pEquipmentType,
                                                    const MFDManager::FileSetInfo*  pFileSetInfo ) const;
    bool                    asgCatalogNewTapeSetCycle( const Context&           context,
                                                       const UINT32             options,
                                                       const FieldList&         additionalFields,
                                                       const FileSpecification& fileSpecification,
                                                       const EquipmentType*     pEquipmentType ) const;
    bool                    asgCatalogNewUnknown( const Context&                    context,
                                                  const UINT32                      options,
                                                  const FieldList&                  additionalFields,
                                                  const FileSpecification&          fileSpecification,
                                                  const MFDManager::FileSetInfo*    pFileSetInfo ) const;
    bool                    asgExisting( const Context&             context,
                                         const UINT32               options,
                                         const FileSpecification&   fileSpecification,
                                         const FieldList&           additionalFields ) const;
    bool                    asgExistingAbsolute( const Context&             context,
                                                 const UINT32               options,
                                                 const FileSpecification&   fileSpecification,
                                                 const FieldList&           additionalFields ) const;
    bool                    asgExistingAlreadyAssigned( const Context&              context,
                                                        const UINT32                options,
                                                        const FileSpecification&    fileSpecification,
                                                        const FieldList&            additionalFields,
                                                        const CITFACITEMS           itFacItem ) const;
    bool                    asgExistingAlreadyAssignedDisk( const Context&              context,
                                                            const UINT32                options,
                                                            const FileSpecification&    fileSpecification,
                                                            const FieldList&            additionalFields,
                                                            const CITFACITEMS           itFacItem ) const;
    bool                    asgExistingAlreadyAssignedDiskCheck( const Context&                                 context,
                                                                 const UINT32                                   options,
                                                                 const FileSpecification&                       fileSpecification,
                                                                 const FieldList&                               additionalFields,
                                                                 const CITFACITEMS                              itFacItem,
                                                                 const INDEX                                    fileEntryIndex,
                                                                 const MFDManager::MassStorageFileCycleInfo&    fileCycleInfo,
                                                                 AlreadyAssignedChangesDisk* const              pChanges ) const;
    bool                    asgExistingAlreadyAssignedTape( const Context&              context,
                                                            const UINT32                options,
                                                            const FileSpecification&    fileSpecification,
                                                            const FieldList&            additionalFields,
                                                            const CITFACITEMS           itFacItem ) const;
    bool                    asgExistingNew( const Context&                  context,
                                            const UINT32                    options,
                                            const FileSpecification&        fileSpecification,
                                            const FieldList&                additionalFields,
                                            const MFDManager::FileSetInfo&  fileSetInfo,
                                            const INDEX                     fileCycleEntryIndex ) const;
    bool                    asgExistingNewDisk( const Context&                              context,
                                                const UINT32                                options,
                                                const FileSpecification&                    fileSpecification,
                                                const FieldList&                            additionalFields,
                                                const MFDManager::FileSetInfo&              fileSetInfo,
                                                const MFDManager::MassStorageFileCycleInfo& fileCycleInfo ) const;
    bool                    asgExistingNewDiskCheck( const Context&                                 context,
                                                     const UINT32                                   options,
                                                     const FileSpecification&                       fileSpecification,
                                                     const FieldList&                               additionalFields,
                                                     const MFDManager::FileSetInfo&                 fileSetInfo,
                                                     const MFDManager::MassStorageFileCycleInfo&    fileCycleInfo,
                                                     NewlyAssignedChangesDisk* const                pChanges,
                                                     bool* const                                    pReadKeyNeeded,
                                                     bool* const                                    pWriteKeyNeeded,
                                                     bool* const                                    pReadInhibited,
                                                     bool* const                                    pWriteInhibited ) const;
    bool                    asgExistingNewDiskExec( const Context&                              context,
                                                    const UINT32                                options,
                                                    const FileSpecification&                    fileSpecification,
                                                    const FieldList&                            additionalFields,
                                                    const MFDManager::FileSetInfo&              fileSetInfo,
                                                    const MFDManager::MassStorageFileCycleInfo& fileCycleInfo,
                                                    const NewlyAssignedChangesDisk&             mfdChanges,
                                                    const bool                                  readKeyNeeded,
                                                    const bool                                  writeKeyNeeded,
                                                    const bool                                  readInhibited,
                                                    const bool                                  writeInhibited ) const;
    bool                    asgExistingNewTape( const Context&                          context,
                                                const UINT32                            options,
                                                const FileSpecification&                fileSpecification,
                                                const FieldList&                        additionalFields,
                                                const MFDManager::FileSetInfo&          fileSetInfo,
                                                const MFDManager::TapeFileCycleInfo&    fileCycleInfo ) const;
    bool                    asgExistingNewTapeCheck( const Context&                         context,
                                                     const UINT32                           options,
                                                     const FileSpecification&               fileSpecification,
                                                     const FieldList&                       additionalFields,
                                                     const MFDManager::FileSetInfo&         fileSetInfo,
                                                     const MFDManager::TapeFileCycleInfo&   fileCycleInfo,
                                                     bool* const                            pReadKeyNeeded,
                                                     bool* const                            pWriteKeyNeeded,
                                                     bool* const                            pReadInhibited,
                                                     bool* const                            pWriteInhibited,
                                                     UINT16* const                          pExpirationPeriod,
                                                     TapeFormat* const                      pFormat,
                                                     char* const                            pLogicalChannel,
                                                     LSTRING* const                         pReelNumbers,
                                                     std::vector<INDEX>* const              pUnitSelectedIndices ) const;
    bool                    asgExistingNewTapeExec( const Context&                          context,
                                                    const UINT32                            options,
                                                    const FileSpecification&                fileSpecification,
                                                    const FieldList&                        additionalFields,
                                                    const MFDManager::FileSetInfo&          fileSetInfo,
                                                    const MFDManager::TapeFileCycleInfo&    fileCycleInfo,
                                                    const bool                              readKeyNeeded,
                                                    const bool                              writeKeyNeeded,
                                                    const bool                              readInhibited,
                                                    const bool                              writeInhibited,
                                                    const UINT16                            expirationPeriod,
                                                    const TapeFormat                        format,
                                                    const char                              logicalChannel,
                                                    const LSTRING&                          reelNumbers,
                                                    const std::vector<INDEX>&               unitSelectedIndices ) const;
    bool                    asgExistingRelative( const Context&             context,
                                                 const UINT32               options,
                                                 const FileSpecification&   fileSpecification,
                                                 const FieldList&           additionalFields ) const;
    bool                    asgPostProcessing( const Context&               context,
                                               const FileSpecification&     fileSpecification,
                                               FacilityItem* const          pFacItem ) const;
    bool                    asgTemporary( const Context&            context,
                                          const UINT32              options,
                                          const FileSpecification&  effectiveSpec,
                                          const FieldList&          additionalFields ) const;
    bool                    asgTemporaryAlreadyAssigned( const Context&             context,
                                                         const UINT32               options,
                                                         const FileSpecification&   effectiveSpec,
                                                         const FieldList&           additionalFields,
                                                         const CITFACITEMS          itFacItem ) const;
    bool                    asgTemporaryAlreadyAssignedDisk( const Context&             context,
                                                             const UINT32               options,
                                                             const FileSpecification&   effectiveSpec,
                                                             const FieldList&           additionalFields,
                                                             const CITFACITEMS          itFacItem ) const;
    bool                    asgTemporaryAlreadyAssignedTape( const Context&             context,
                                                             const UINT32               options,
                                                             const FileSpecification&   effectiveSpec,
                                                             const FieldList&           additionalFields,
                                                             const CITFACITEMS          itFacItem ) const;
    bool                    asgTemporaryNew( const Context&             context,
                                             const UINT32               options,
                                             const FileSpecification&   effectiveSpec,
                                             const FieldList&           additionalFields ) const;
    bool                    asgTemporaryNewDisk( const Context&             context,
                                                 const UINT32               options,
                                                 const FileSpecification&   effectiveSpec,
                                                 const FieldList&           additionalFields,
                                                 const EquipmentType* const pEquipmentType ) const;
    bool                    asgTemporaryNewTape( const Context&             context,
                                                 const UINT32               options,
                                                 const FileSpecification&   effectiveSpec,
                                                 const FieldList&           additionalFields,
                                                 const EquipmentType* const pEquipmentType ) const;
    bool                    asgUnspecified( const Context&              context,
                                            const UINT32                options,
                                            const FileSpecification&    effectiveSpec,
                                            const FieldList&            additionalFields ) const;
    bool                    catDisk( const Context&                 context,
                                     const UINT32                   options,
                                     const FieldList&               additionalFields,
                                     FileSpecification* const       pEffectiveSpec,
                                     const EquipmentType*           pEquipmentType,
                                     const MFDManager::FileSetInfo* pFileSetInfo ) const;
    bool                    catDiskCycle( const Context&                    context,
                                          const UINT32                      options,
                                          const FieldList&                  additionalFields,
                                          FileSpecification* const          pEffectiveSpec,
                                          const EquipmentType*              pEquipmentType,
                                          const MFDManager::FileSetInfo*    pFileSetInfo ) const;
    bool                    catDiskSetCycle( const Context&             context,
                                             const UINT32               options,
                                             const FieldList&           additionalFields,
                                             FileSpecification* const   pEffectiveSpec,
                                             const EquipmentType*       pEquipmentType ) const;
    bool                    catTape( const Context&                 context,
                                     const UINT32                   options,
                                     const FieldList&               additionalFields,
                                     FileSpecification* const       pEffectiveSpec,
                                     const EquipmentType*           pEquipmentType,
                                     const MFDManager::FileSetInfo* pFileSetInfo ) const;
    bool                    catTapeCycle( const Context&                    context,
                                          const UINT32                      options,
                                          const FieldList&                  additionalFields,
                                          FileSpecification* const          pEffectiveSpec,
                                          const EquipmentType*              pEquipmentType,
                                          const MFDManager::FileSetInfo*    pFileSetInfo ) const;
    bool                    catTapeSetCycle( const Context&             context,
                                             const UINT32               options,
                                             const FieldList&           additionalFields,
                                             FileSpecification* const   pEffectiveSpec,
                                             const EquipmentType*       pEquipmentType ) const;
    bool                    catUnknown( const Context&                  context,
                                        const UINT32                    options,
                                        const FieldList&                additionalFields,
                                        FileSpecification* const        pEffectiveSpec,
                                        const MFDManager::FileSetInfo*  pFileSetInfo ) const;
    bool                    checkCycleConstraints( const Context&           context,
                                                   const FileSpecification& fileSpecification ) const;
    bool                    checkCycleConstraints( const Context&                   context,
                                                   const FileSpecification&         effectiveFileSpec,
                                                   const MFDManager::FileSetInfo&   fileSetInfo,
                                                   UINT16* const                    pAbsoluteCycle ) const;
    bool                    checkDiskEquipmentType( const Context&                                      context,
                                                    const FieldList&                                    additionalFields,
                                                    const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
                                                    const FacilityItem* const                           pFacItem,
                                                    DiskSubfieldInfo* const                             pInfo ) const;
    bool                    checkDiskInitialReserve( const Context&                                     context,
                                                     const FieldList&                                   additionalFields,
                                                     const MFDManager::MassStorageFileCycleInfo* const  pFileCycleInfo,
                                                     const FacilityItem* const                          pFacItem,
                                                     DiskSubfieldInfo* const                            pInfo ) const;
    bool                    checkDiskSubfields( const Context&                                      context,
                                                const FieldList&                                    additionalFields,
                                                const MFDManager::MassStorageFileCycleInfo* const   pFileCycleInfo,
                                                const FacilityItem* const                           pFacItem,
                                                DiskSubfieldInfo* const                             pInfo,
                                                const bool                                          catalogRequest ) const;
    const EquipmentType*    checkEquipmentType( const Context&      context,
                                                const FieldList&    additionalFields ) const;
    bool                    checkPrivate( const Context&                            context,
                                          const MFDManager::FileSetInfo* const      pFileSetInfo,
                                          const MFDManager::FileCycleInfo* const    pFileCycleInfo ) const;
    bool                    checkTapeSubfields( const Context&                              context,
                                                const FieldList&                            additionalFields,
                                                const MFDManager::FileSetInfo* const        pFileSetInfo,
                                                const MFDManager::TapeFileCycleInfo* const  pFileCycleInfo,
                                                const FacilityItem* const                   pFacItem,
                                                TapeSubfieldInfo* const                     pInfo ) const;
    bool                    checkZOption( const Context&    context,
                                          const UINT32      options ) const;
    const EquipmentType*    findEquipmentType( const std::string& mnemonic ) const;
    bool                    findLeadAndMainItemInfoDisk( const FileSpecification&                       fileSpecification,
                                                         MFDManager::FileSetInfo* const                 pFileSetInfo,
                                                         INDEX* const                                   pCycleEntryIndex,
                                                         MFDManager::MassStorageFileCycleInfo* const    pFileCycleInfo ) const;
    bool                    findLeadItemInfo( const FileSpecification&          fileSpecification,
                                              MFDManager::FileSetInfo* const    pFileSetInfo,
                                              INDEX* const                      pCycleEntryIndex ) const;
    bool                    freeInternalName( const Context&        context,
                                              const UINT32          options,
                                              const SuperString&    internalName ) const;
    bool                    freeReleaseDiskFile( const Context&             context,
                                                 DiskFacilityItem* const    pFacilityItem,
                                                 const bool                 deleteFlag ) const;
    bool                    freeReleaseDiskUnusedInitialReserve( const Context&             context,
                                                                 DiskFacilityItem* const    pFacilityItem ) const;
    bool                    freeReleaseFacilityItem( const Context&         context,
                                                     const UINT32           options,
                                                     FacilityItem* const    pFacilityItem ) const;
    bool                    freeReleaseTapeFile( const Context&             context,
                                                 TapeFacilityItem* const    pFacilityItem,
                                                 const bool                 deleteFlag ) const;

    //  Private static methods
    static void                     asgCheckForDLOC$( const Context&                    context,
                                                      const FileSpecification&          fileSpecification,
                                                      const StandardFacilityItem* const pFacItem );
    static void                     asgResolveNameItems( const Context&                     context,
                                                         const StandardFacilityItem* const  pFacItem );
    static bool                     checkDisables( const Context&                   context,
                                                   const UINT32                     options,
                                                   const bool                       diskFlag,
                                                   const MFDManager::FileCycleInfo& fileCycleInfo,
                                                   bool* const                      pReadDisabled,
                                                   bool* const                      pWriteDisabled );                                                        
    static bool                     checkDiskGranularity( const Context&                                    context,
                                                          const FieldList&                                  additionalFields,
                                                          const MFDManager::MassStorageFileCycleInfo* const pFileCycleInfo,
                                                          const FacilityItem* const                         pFacItem,
                                                          DiskSubfieldInfo* const                           pInfo );
    static bool                     checkDiskMaxGranules( const Context&                                    context,
                                                          const FieldList&                                  additionalFields,
                                                          const MFDManager::MassStorageFileCycleInfo* const pFileCycleInfo,
                                                          const FacilityItem* const                         pFacItem,
                                                          DiskSubfieldInfo* const                           pInfo );
    static bool                     checkDiskPlacement( const Context&      context,
                                                        const FieldList&    additionalFields,
                                                        const bool          catalogRequest );
    static bool                     checkForExistingCycle( const Context&                   context,
                                                           const FileSpecification&         fileSpecification,
                                                           const MFDManager::FileSetInfo&   fileSetInfo,
                                                           const bool                       asgExisting );
    static bool                     checkKeys( const Context&                   context,
                                               const FileSpecification&         fileSpecification,
                                               const MFDManager::FileSetInfo&   fileSetInfo,
                                               const bool                       catalogFlag,
                                               bool* const                      pReadDisabled,
                                               bool* const                      pWriteDisabled );
    static bool                     checkMFDResult( const MFDManager::Result        mfdResult,
                                                    Result* const                   pFacilitiesResult );
    static bool                     checkOptionConflicts( const Context&    context,
                                                          const UINT32      optionsGiven,
                                                          const UINT32      mutuallyExclusiveOptions );
    static bool                     checkOptions( const UINT32          optionsGiven,
                                                  const UINT32          allowedOptions,
                                                  Result* const         pResult = 0 );
    static bool                     checkTempFileAssigned( const Context&           context,
                                                           const FileSpecification& fileSpec,
                                                           CITFACITEMS* const       pItFacItem );
    static bool                     compareFileSpecificationToFacilityItem( const FileSpecification&    fileSpecification,
                                                                            const FacilityItem* const   pFacilityItem );
    static bool                     findExistingCycleEntryIndex( const FileSpecification&           fileSpecification,
                                                                 const MFDManager::FileSetInfo&     fileSetInfo,
                                                                 INDEX* const                       pIndex );
    static const StatusCodeInfo*    findStatusCodeInfo( const StatusCode statusCode );
    static void                     freeReleaseNameItems( const Context&            context,
                                                          FacilityItem::IDENTIFIER  facilityItemIdentifier );
    static void                     freeUnresolveNameItems( const Context&              context,
                                                            FacilityItem::IDENTIFIER    facilityItemIdentifier );
    static void                     getEffectiveFileSpecification( const RunInfo* const     pRunInfo,
                                                                   const FileSpecification& originalSpecification,
                                                                   FileSpecification* const pEffectiveSpecification );
    static bool                     isFileAssigned( const FileSpecification&    fileSpecification,
                                                    const RunInfo&              runInfo,
                                                    CITFACITEMS* const          pIterator );
    static void                     logMFDError( const std::string&         locus,
                                                 const FileSpecification&   fileSpecification,
                                                 const MFDManager::Result&  mfdResult );
    static void                     logMFDError( const Context&             context,
                                                 const std::string&         locus,
                                                 const FileSpecification&   fileSpecification,
                                                 const MFDManager::Result&  mfdResult );
    static void                     logMFDError( const Context&             context,
                                                 const std::string&         locus,
                                                 const FacilityItem* const  pFacilityItem,
                                                 const MFDManager::Result&  mfdResult );
    static const SuperString        resolveQualifier( const RunInfo* const      pRunInfo,
                                                      const FileSpecification&  fileSpecification );


public:
    FacilitiesManager( Exec* const pExec );

    Result                          asg( Activity* const            pActivity,
                                         SecurityContext* const     pContext,
                                         RunInfo* const             pRunInfo,
                                         const UINT32               options,
                                         const FileSpecification&   fileSpecification,
                                         const FieldList&           additionalFields );
    Result                          cat( Activity* const            pActivity,
                                         SecurityContext* const     pContext,
                                         RunInfo* const             pRunInfo,
                                         const UINT32               options,
                                         const FileSpecification&   fileSpecification,
                                         const FieldList&           additionalFields );
    Result                          free( Activity* const           pActivity,
                                          SecurityContext* const    pContext,
                                          RunInfo* const            pRunInfo,
                                          const UINT32              options,
                                          const FileSpecification&  fileSpecification );
    Result                          log( Activity* const                pActivity,
                                         const SecurityContext* const   pContext,
                                         RunInfo* const                 pRunInfo,
                                         const std::string&             message ) const;
    Result                          qual( RunInfo* const            pRunInfo,
                                          const UINT32              options,
                                          const std::string&        qualifier ) const;
    void                            releaseFacilityItemRunTermination( Activity* const          pActivity,
                                                                       SecurityContext* const   pSecurityContext,
                                                                       RunInfo* const           pRunInfo,
                                                                       FacilityItem::IDENTIFIER identifier,
                                                                       const bool               errorTermFlag ) const;
    Result                          use( RunInfo* const             pRunInfo,
                                         const UINT32               options,
                                         const FileSpecification&   specification1,
                                         const FileSpecification&   specification2 ) const;

	//  ExecManager interface
    void                            dump( std::ostream&     stream,
                                          const DUMPBITS    dumpBits );
    void                            shutdown();
    bool                            startup();
    void                            terminate();

    //  statics
    inline static const EquipmentType*  getDefaultMassStorageEquipmentType()
    {
        return m_pDefaultEquipMassStorage;
    }
};


std::ostream& operator<< ( std::ostream&, const FacilitiesManager::Field& );
std::ostream& operator<< ( std::ostream&, const FacilitiesManager::FieldList& );



#endif
