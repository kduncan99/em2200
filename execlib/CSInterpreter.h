//  CSInterpreter
//  Copyright (c) 2015 by Kurt Duncan
//
//  General control statement interpreter


#ifndef     EXECLIB_CS_INTERPRETER_H
#define     EXECLIB_CS_INTERPRETER_H



#include    "Activity.h"
#include    "ControlModeRunInfo.h"
#include    "FacilitiesManager.h"
#include    "FileSpecification.h"



class   CSInterpreter
{
public:
    enum Command
    {
        CMD_INIT,
        CMD_EMPTY,
        CMD_LABEL,

        //CMD_ADD,
        CMD_ASG,
        //CMD_BRKPT,
        CMD_CAT,
        //CMD_CKPT,
        CMD_DIR,
        CMD_END,
        CMD_ENDF,
        CMD_ENDX,
        CMD_EOF,
        CMD_FILE,
        CMD_FIN,
        CMD_FREE,
        //CMD_HDG,
        //CMD_JUMP,
        CMD_LOG,
        //CMD_MODE,
        CMD_MSG,
        //CMD_PASSWD, ? i think this is only scanned in st and @start...
        CMD_QUAL,
        CMD_RINFO,
        //CMD_RSTRT,
        CMD_RUN,
        CMD_SETC,
        //CMD_START,
        //CMD_SYM,
        //CMD_TEST,
        CMD_USE,
        //CMD_XQT,
    };

    enum SetcLogicalOp
    {
        SLOP_NONE,
        SLOP_AND,
        SLOP_OR,
        SLOP_XOR,
    };

    enum SetcPartialWord
    {
        SPAW_NONE,
        SPAW_T1,
        SPAW_T2,
        SPAW_S2,
        SPAW_S3,
        SPAW_S4,
    };

    enum Status
    {
        //  Status returned on both interpretStatement() and executeStatement()
        CSIST_SUCCESSFUL,                   //  Valid statement which succeeded
        CSIST_INVALID_OPTION,               //  An invalid option was specified
        CSIST_INVALID_OPTION_COMBINATION,   //  Mutually exclusive options specified

        //  Status returned only on interpretStatement()
        CSIST_CONTINUED,                    //  Interpretation stalled by continuation sentinel - read another image
        CSIST_EXTRANEOUS_TEXT,              //  After completion of scanning, more text was found in the input image
        CSIST_ILLEGAL_CONTINUATION,         //  Continuation character provided incorrectly
        CSIST_ILLEGAL_OPTION,               //  An illegal option was specified; see m_Index for position of option
        CSIST_INVALID_ACCOUNT_ID,           //  An account-id was found, but it contains a syntax error
        CSIST_INVALID_CYCLE,                //  A relative or absolute cycle was found, but it contains a syntax error
        CSIST_INVALID_DIRECTORY_ID,         //  A directory ID was found, but it contains a syntax error
        CSIST_INVALID_FILE_NAME,            //  A filename was found, but it contains a syntax error
        CSIST_INVALID_INTERNAL_NAME,        //  A field which should be an internal filename is invalid (maybe a full filename spec)
        CSIST_INVALID_KEY,                  //  A read or write key was found, but it contains some sort of syntax error
        CSIST_INVALID_LABEL,                //  A label was found, but it contains a syntax error
        CSIST_INVALID_PROJECT_ID,           //  A project-id was found, but it contains a syntax error
        CSIST_INVALID_QUALIFIER,            //  A qualifier was found, but it contains a syntax error
        CSIST_INVALID_RUN_ID,               //  A run-id was found, but it contains a syntax error
        CSIST_INVALID_SUBFIELD,             //  A general subfield in a command statement is invalid (i.e., not 0-12 chars/digits)
        CSIST_INVALID_USER_ID,              //  A user-id was found, but it contains a syntax error
        CSIST_KEYS_NOT_ALLOWED,             //  Read/Write keys are not allowed where the user specified them
        CSIST_LABEL_NOT_ALLOWED,            //  A label was encountered in a CSF/ACSF/CSI image (non-control mode)
        CSIST_MAX_FIELDS_SUBFIELDS,         //  User specified too many fields or subfields
        CSIST_MAX_SUBFIELD_CHARS,           //  A specified subfield contains too many characters
        CSIST_MISSING_FILE_SPECIFICATION,   //  A required file specification is missing
        CSIST_NOT_CONTROL_STATEMENT,        //  No masterspace in first column
        CSIST_NOT_FOUND,                    //  Whatever we were looking for, we didn't find it.
                                            //      Externally, this means the statement is not a recognized control statement,
                                            //      although it *might* be a processor call, and it does contain a proper masterspace.
        CSIST_SYNTAX_ERROR,                 //  General syntax error

        //  Status returned only on executeStatement()
        CSIST_FACILITIES_RESULT,            //  Facilities has status information (could be successful or not)
        CSIST_INTERRUPTED,                  //  During execution, caller invoked stopExecution()
        CSIST_INVALID_COMMAND_CODE,         //  execute was called after a failed interpret, or without calling interpret,
                                            //      or after interpreting an empty statement or a label statement.
        CSIST_NOT_ALLOWED,                  //  Statement not allowed for the particular RunInfo given
    };


private:
    //  Useful pointers
    ConsoleManager* const           m_pConsMgr;
    Exec* const                     m_pExec;
    FacilitiesManager* const        m_pFacMgr;
    MFDManager* const               m_pMfdMgr;

    SuperString                     m_AccountId;                    //  for RUN
    Activity*                       m_pActivity;                    //  pointer to requesting Activity
    FacilitiesManager::FieldList    m_AdditionalFields;             //  for ASG or CAT
    bool                            m_AllowContinuation;
    bool                            m_AllowLabel;
    Command                         m_Command;
    bool                            m_CommentFlag;                  //  image contains commentary
    bool                            m_DemandFlag;                   //  interpret for DEMAND (not used much, but for @RUN)
    FacilitiesManager::Result       m_FacilitiesResult;
    FileSpecification               m_FileSpec1;
    FileSpecification               m_FileSpec2;
    LSTRING                         m_Images;                       //  original images
    INDEX                           m_Index;
    SuperString                     m_Label;
    SetcLogicalOp                   m_LogicalOp;                    //  for SETC
    UINT32                          m_Options;
    SetcPartialWord                 m_PartialWord;                  //  for SETC
    char                            m_ProcessorDispatchingPriority; //  for RUN
    SuperString                     m_ProjectId;                    //  for RUN
    SuperString                     m_Qualifier;                    //  for QUAL
    SuperString                     m_RunId;                        //  for RUN
    RunInfo* const                  m_pRunInfo;                     //  pointer to RunInfo object to be operated upon
    char                            m_SchedulingPriority;           //  for RUN
    SecurityContext* const          m_pSecurityContext;             //  context in which this command is to run
    std::string                     m_Statement;                    //  final composite image
    Status                          m_Status;                       //  Status of call to interpret or execute statement
    bool                            m_StopExecutionFlag;
    SuperString                     m_Text;                         //  for MSG or LOG
    SuperString                     m_UserId;                       //  for RUN
    COUNT64                         m_Value;                        //  for SETC, DIR

    Status              executeAsg();
    Status              executeCat();
    Status              executeDir() const;
    Status              executeDirCatalog( UserRunInfo* const pUserRunInfo ) const;
    Status              executeDirDump( UserRunInfo* const pUserRunInfo ) const;
    Status              executeDirList( UserRunInfo* const pUserRunInfo ) const;
    Status              executeFin();
    Status              executeFree();
    Status              executeLog();
    Status              executeMsg();
    Status              executeQual();
    Status              executeRinfo();
    Status              executeRun();
    Status              executeSetc();
    Status              executeUse();
    void                initialize();
    Status              interpret();
    Status              scanAbsoluteFileCycle( UINT16* const pCycle );
    Status              scanAdditionalFields();
    Status              scanBasicFileSpecification( FileSpecification* const pFileSpec );
    Status              scanDirectoryId( std::string* const pDirectoryId );
    Status              scanField( VSTRING* const       pContainer,
                                   std::vector<COUNT>*  pSizeTemplate = 0 );
    Status              scanFields( std::vector<VSTRING>* const             pContainer,
                                    std::vector<std::vector<COUNT>>* const  pTemplate = 0 );
    Status              scanFileName( std::string* const pFileName );
    Status              scanFileSpecification( FileSpecification* const pFileSpec );
    Status              scanLabel();
    Status              scanLogicalOperator( SetcLogicalOp* const pOperator );
    Status              scanMasterSpace();
    Status              scanOctalInteger( COUNT64* const    pValue,
                                          const bool        requireLeadingZero = true,
                                          const COUNT       maxDigits = 12 );
    Status              scanOptions( UINT32* const  pMask,
                                     const UINT32   allowMask = 0377777777 );
    Status              scanPartialWord( SetcPartialWord* const pPartialWord );
    Status              scanPriority( char* pPriority );
    Status              scanQualifier( std::string* const   pQualifier,
                                       const bool           scanAsterisk = true );
    Status              scanReadWriteKey( std::string* const pKey );
    Status              scanRelativeFileCycle( INT16* const pCycle );
    Status              scanRemainder();
    Status              scanRemainderAsg();
    Status              scanRemainderCat();
    Status              scanRemainderDir();
    Status              scanRemainderEnd();
    Status              scanRemainderEndf();
    Status              scanRemainderEndx();
    Status              scanRemainderEof();
    Status              scanRemainderFin();
    Status              scanRemainderFree();
    Status              scanRemainderLog();
    Status              scanRemainderMsg();
    Status              scanRemainderQual();
    Status              scanRemainderRinfo();
    Status              scanRemainderRun();
    Status              scanRemainderSetc();
    Status              scanRemainderUse();
    Status              scanRunOptionsAndPriorities();
    Status              scanString( const std::string&  testString,
                                    const bool          caseSensitive = true );
    Status              scanSubField( std::string* const    pSubField,
                                        const COUNT         maxChars = 0 );
    Status              scanWhiteSpace( const bool allowContinuation = true );

    bool                atEnd() const;
    bool                atSubFieldTerminator() const;

    static inline bool  isValidSubfield( const std::string& subfieldStr );


public:
    //  Caller must have filtered out comments, and properly concatenated continuation images
    //  before constructing the object.
    CSInterpreter( Exec* const              pExec,
                   Activity* const          pRequestingActivity,
                   SecurityContext* const   pSecurityContext,
                   RunInfo* const           pRunInfo );
    ~CSInterpreter();

    bool                decomposeIndex( const INDEX     index,
                                        INDEX* const    pImageIndex,
                                        INDEX* const    pColumnIndex );
    Status              executeStatement( Activity* const pActivity );
    Status              interpretStatement( const std::string&  singleStatement,
                                            const bool          demandRun,
                                            const bool          allowContinuation,
                                            const bool          allowLabel );
    Status              interpretStatementStack( const LSTRING& imageStack,
                                                 const bool     demandRun,
                                                 const bool     allowContinuation,
                                                 const bool     allowLabel );
    bool                isErrorStatus() const;
    void                postExecuteStatusToPrint( ControlModeRunInfo* const pRunInfo ) const;
    void                postInterpretStatusToPrint( ControlModeRunInfo* const pRunInfo ) const;

    inline const SuperString&                   getAccountId() const        { return m_AccountId; }
    inline const FacilitiesManager::FieldList&  getAdditionalFields() const { return m_AdditionalFields; }
    inline Command                              getCommand() const          { return m_Command; }
    inline bool                                 getCommentFlag() const      { return m_CommentFlag; }
    inline char                                 getCurrentCharacter() const { return m_Statement[m_Index]; }
    inline const FacilitiesManager::Result&     getFacilitiesResult() const { return m_FacilitiesResult; }
    inline const FileSpecification&             getFileSpec1() const        { return m_FileSpec1; }
    inline const FileSpecification&             getFileSpec2() const        { return m_FileSpec2; }
    inline INDEX                                getIndex() const            { return m_Index; }
    inline const std::string&                   getLabel() const            { return m_Label; }
    inline SetcLogicalOp                        getLogicalOp() const        { return m_LogicalOp; }
    inline UINT32                               getOptions() const          { return m_Options; }
    inline SetcPartialWord                      getPartialWord() const      { return m_PartialWord; }
    inline char                                 getProcessorDispatchingPriority() const
                                                                            { return m_ProcessorDispatchingPriority; }
    inline const SuperString&                   getProjectId() const        { return m_ProjectId; }
    inline const SuperString&                   getQualifier() const        { return m_Qualifier; }
    inline const SuperString&                   getRunId() const            { return m_RunId; }
    inline char                                 getSchedulingPriority() const
                                                                            { return m_SchedulingPriority; }
    inline Status                               getStatus() const           { return m_Status; }
    inline const std::string&                   getText() const             { return m_Text; }
    inline const SuperString&                   getUserId() const           { return m_UserId; }
    inline COUNT64                              getValue() const            { return m_Value; }
    inline void                                 stopExecution()             { m_StopExecutionFlag = true; }

    static const char*                          getSetcLogicalOpString( const SetcLogicalOp logicalOp );
    static const char*                          getSetcPartialWordString( const SetcPartialWord partialWord );
    static std::string                          getStatusString( const Status status );
};


#ifdef  _DEBUG
std::ostream&       operator<< ( std::ostream& stream, const CSInterpreter& csInterpreter );
#endif



#endif
