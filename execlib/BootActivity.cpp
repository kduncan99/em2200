//	BootActivity.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	This is the code which runs at boot time, and starts up all the EXEC stuff.
//
//  We are responsible for creating, directly or through SYS, the following system files:
//      SYS$*DLOC$          Security privilege file
//      SYS$*GENF$          General storage file
//      SYS$*LIB$           System absolute library
//      SYS$*RLIB$          System relocatable library (incl. procs, IIRC)
//      SYS$*RUN$           System runstream library
//      SYS$*PRINTER$       Printer microcode - obsolete, but we create an empty file for the heck of it



#include    "execlib.h"



//  local statics

static  Word36      Routing;



/*
5.1. System Control Files
OS 2200 software establishes and uses the following system control files:
  SYS$*LIB$
  SYS$*RUN$
  SYS$*RLIB$
  SYS$*PRINTER$
  SYS$*DATA$
  SYS$LIB$*RUN$
  SYS$*GENF$
  SYS$*ILES$

5.1.1. System Library File (SYS$*LIB$)
File SYS$*LIB$ contains system processors and reentrant code banks available to
programs running under the Exec operating system. SYS$*LIB$ is cataloged by
predefined runstream element SYS during a boot with jump key 4 (or jump keys 4 and
13) already set. SYS$*LIB$ is the fourth file on the boot tape and is in @COPY,G format.
It can be modified only with a change to the fourth file on the Exec boot tape, and by
performing a tape boot with the jump key 4 set.

5.1.2. Runstream File (SYS$*RUN$)
File SYS$*RUN$ is cataloged by predefined runstream SYS during a boot with jump key
4 (or jump keys 4 and 13) set. It is copied from the fifth file on the boot tape, which is in
@COPY,G format. RUN$ is a program file that consists of a number of symbolic and
absolute elements.

The one element that must be present in this file is the symbolic element called
BOOTELT. This element is a partial runstream, which is added (@ADD) and executed as
part of the SYS run. Its functions in the SYS run are the following:
    Catalogs and loads SYS$*RLIB$ from the sixth file on the boot tape (if a jump key 4,
        or a jump key 4 and 13 boot is taking place)
    Deletes the old SYS$*SYS$MAP, and catalogs and loads a new SYS$*SYS$MAP
        from the second or eighth file of the boot tape (if a tape boot is taking place)
    Loads SYS$*PRINTER$ from the seventh file on the boot tape (if a jump key 4 boot is taking place)

The determination of the type of boot taking place is made by testing the condition word,
which an @SETC control statement sets up in the beginning of the SYS run. The @SETC
control statement is one of the executable control statements that exists in the Exec for
the SYS run. Before starting the SYS run, the Exec modifies the @SETC control
statement by setting up the �value� part of the control statement to reflect the type of
boot operation taking place. The following information shows the condition word values
and their associated meanings for S3 and S4.
Values in S3:
00 Disk boot.
01 Tape boot.
02 Unused.
04 File sharing is configured.
Values in S4:
01 Jump key 4 set.
02 Jump key 13 set.
010 FLIT BOOT to suppress @START COSM.
020 FAS installed.
040 Absolute-only delivery.
Condition word values for combinations of these switches are determined by adding the
values for individual switches.
The Exec also has an @ASG image used by SYS to assign (create) the SYS$*RUN$ file.
The full image is as follows:
@ASG,CPGZ SYS$*RUN$//RUNWR$.,F2/1//10000

Other elements in SYS$*RUN$ are the following:
    CONSOL
        This runstream and absolute perform the execution of an interactive run at the operator's console.
    DFILES
        This runstream calls the File Administration System (FAS) to list all disabled files.
        This runstream is started (@START) by SYS RUN on any recovery boot.
    DPREP
        This runstream initiates a disk prep.
    DLLLOD
        This runstream initiates a downline load.
    LIBSAVE
        This runstream copies all installed software program products to a library save tape
        that COMUS has formatted.
    LIBLOAD
        This runstream installs all software program products from a library save tape that
        COMUS has formatted onto the running system. It can be used after a jump key 13 boot.
    INSTALL
        This runstream installs new products from a software product master tape that has
        been formatted for COMUS.

The RUN$ file has a write key on it. If you want to restrict access to this file, you should
make local changes to the write key. A change to the write key for RUN$ requires
changes to the canned part of the SYS run in internal Exec element INDRIV, as well as to
element INASG. You should ensure the use of a read key for SYS$*RUN$ to protect the
write keys for the RLIB$ and PRINTER$ files.

5.1.3. Relocatable Library File (SYS$*RLIB$)
File SYS$*RLIB$ is cataloged by predefined runstream element SYS during a system
boot with jump key 4 (or jump keys 4 and 13) set. It contains relocatable libraries that
the Collector (MAP processor) uses to create absolute user programs.
The RLIB$ file has a write key on it. If you want to restrict access to this file, you should
make local changes to the write key. A change to the write key for the RLIB$ file
requires changes to element SYS$*RUN$.BOOTELT to reflect the new write key.

5.1.4. Data File (SYS$*DATA$)
The SYS$*DATA$ file contains several elements that the Exec, the Collector, and
COMUS use to control various phases of the system libraries. COMUS creates and
maintains SYS$*DATA$. SYS$*DATA$ must always be cataloged on fixed mass
storage. The control elements are as follows:
    FILE-BDI$ (omnibus element)
        The COMUS INSTALL feature maintains this element. It consists of a group of file
        name/bank descriptor index (BDI) pairs. For each common bank installed into file X,
        the bank BDI and location (X) are placed in FILE-BDI$. Additionally, omnibus element
        BANK-BDI$ is placed in file X, which contains information previously encoded in
        REPROG stream generation statements (SGS).
    LIB-ABS$ (omnibus element)
        This element (maintained by COMUS) contains the list of processor names and their
        locations (file name) that have been registered with the Exec. It lets you insert a
        processor into any file and invoke the processor.
    LIB$NAMES (omnibus element)
        The Collector (MAP processor) uses this element (maintained by COMUS). It
        consists of a set of file names that are to be searched to resolve external references.
    CO$INSTALL$/COMUS$ (symbolic element)
        COMUS maintains this element. During software installation, COMUS re-creates the
        three previously mentioned omnibus elements from the data in this element.
    CO$ACRNAME$/COMUS$ (optional symbolic element)
        COMUS creates this element only if COMUS SECURITY and COMUS SENTRY
        options are enabled. It consists of the set of access control record (ACR) names and
        user-ids COMUS generates. See the COMUS End Use Reference Manual.
    CO$INSTALL$/HISTORY (symbolic element)
        This element (maintained by COMUS) contains information on products that have
        been removed from the system or replaced by subsequent levels of the same
        product.
    SSDEF$ (absolute element)
        This element (maintained by COMUS) is created by the Subsystem Definition
        Processor (SSDP). The linking system searches it when resolving references during
        execution.
    SHARED-ELTS (symbolic element)
        This element (maintained by COMUS) contains control information about elements
        that are shared by products.

5.1.5. Library Runstream File (SYS$LIB$*RUN$)
SYS$LIB$*RUN$ holds runstreams generated by the site or provided by Unisys. When
the operator issues an ST keyin, the Exec searches for the runstream in
SYS$LIB$*RUN$. If that search fails, system file SYS$*RUN$ is searched.
COMUS maintains the symbolic element SYS$LIB$*RUN$.AUTO$START, which
contains the names of the runstreams that are automatically started by the SYS run at
the end of boots.
To copy your own runstreams into SYS$LIB$*RUN$, you can use the FURPUR @COPY
command. Specify a unique element name to prevent destruction of background
runstreams that Unisys provides. Use the following keyin to start the local runstreams
from the console:
ST element-name
where element-name is the name of the runstream copied into SYS$LIB$*RUN$.
To start runstreams with system high security attributes by using the ST keyin, you must
use the Z option. Refer to the Security Administration for ClearPath OS 2200 Help for
additional information on maintaining runstreams in SYS$LIB$*RUN$ if your site has
Security Option 1, 2, or 3.
The SYS$LIB$*RUN$ file is protected by a write key established when COMUS catalogs
this file. To modify the contents of this file, one of the following must be true:
    You must have assigned SYS$LIB$*RUN$ with the proper write key.
    You must have SYS$*DLOC$ assigned.
    You must be using the security officer�s user-id.

5.1.6. General Exec File (SYS$*GENF$)
SYS$*GENF$ is a general Exec file that contains the symbiont output queue and the
scheduling queue. It is a cataloged file, allowing recovery of the file. If
SENTRY_CONTROL=FALSE, then SYS$*GENF$ is exclusively assigned to the Exec.

5.1.7. ILES File (SYS$*ILES$)
SYS$*ILES$ contains control information about independently linked Exec subsystems
(ILES). The file contains one element, INSTALD$ILES, which is the directory of all ILES
that are currently installed in the system.
SYS$*ILES$ is cataloged by the Exec on an initial boot. If possible, it is recovered on
recovery boots; otherwise, it is recataloged. File access is controlled by an ACR if
SENTRY_CONTROL is TRUE or by a write key if SENTRY_CONTROL is FALSE. The file
is accessed by the Exec during ILES loading and by the MILES processor, which is used
to manage ILES loading and termination or to report on ILES. When MILES installs or
deinstalls a particular ILES, it updates the file and creates multiple F-cycles. See Section
12 for details on using MILES to manage ILES loading and termination.

5.2. System Security Files�Fundamental System
The following files are associated with the security system: SYS$*DLOC$,
SYS$*SEC@USERID$, SYS$*SEC@ACR$, SYS$*ACCOUNT$R1, SYS$*SEC@ACCTINFO,
SYS$*SMDTF$@@@@@@, and SYS$*MFDF$$. The following subsections describe the
use of these files and the protection mechanisms provided to control access to the data
in these files.
This information applies when SENTRY_CONTROL is set to FALSE (Fundamental
Security). For information when SENTRY_CONTROL is set to TRUE (that is, Security
Option 1, 2, or 3), refer to the Security Administration for ClearPath OS 2200 Help.

5.2.1. System Privilege Switch (SYS$*DLOC$)
When the SYS$*DLOC$ file is assigned to a run with read and write capability (the
correct read and write keys were specified at @ASG time), the run can perform certain
privileged system functions.
The read and write keys are hard-coded values that appear in Exec listings. The keys
should be changed after the SYS FIN message for the initial boot, but before any batch or
demand runs are allowed to run. You can use the FURPUR @CHG command to do this.
Because some runs need the privileges associated with SYS$*DLOC$, those runstreams
need to be controlled to prevent disclosure of the keys. A read key should be used to
control access to files containing these privileged runs.
Note: The Exec secured ER MODPS$ provides equivalent capabilities. See the Exec
ER Programming Reference Manual.

5.2.2. User-id Security record Database (SYS$*SEC@USERID$)
Even with Fundamental Security (SENTRY_CONTROL set to FALSE), the
SYS$*SEC@USERID$ file is created during system initialization to hold user attributes
and is initialized with records for the security officer, INSTALLATION & EXEC8 user-ids.
The security officer user-id is solicited during system initialization if it is not explicitly
configured in the Exec boot tape. For more information, see the Security Administration
for ClearPath OS 2200 Help.

5.2.3. ACR, Privilege, and Interface Security Record Database
(SYS$*SEC@ACR$)
The SYS$*SEC@ACR$ file is created during system initialization to hold Privilege and
CALL/ER interface records. Two system ACRs are created with Fundamental Security, but
they are not used. For more information, see the Security Administration for ClearPath
OS 2200 Help.

5.2.4. Summary Account Files (SYS$*ACCOUNT$R1 and
SYS$*SEC@ACCTINFO)
SYS$*ACCOUNT$R1 is assigned exclusively to the Exec. This file and
SYS$*SEC@ACCTINFO hold Account, Quota, and Account Information records, and are
managed with Security-Admin or SIMAN. Exec configuration parameters affect the use
of this data.
See Section 10 for detailed information on Quota and SYS$*ACCOUNT$R1 and the use
of ER ACCNT$ to access the file.
The file SYS$*SEC@ACCTINFO holds account information for account numbers in
ACCOUNT$R1. For more information, see the Security Administration for ClearPath
OS 2200 Help.

5.2.5. Master File Directory File (SYS$*MFDF$$)
The SYS$*MFDF$$ file contains information about each cataloged file in the system. As
users create files, the security system generates entries in SYS$*MFDF$$ Lead & Main
Items. See Section 2 for additional information.
*/



//	private / protected methods

//  assignSystemFile()
//
//  Assigns one system file to the Exec, based on the current state and the settings for the
//  SystemFileInfo object associated with that state.
bool
BootActivity::assignSystemFile
(
    SystemFileInfo* const   pSystemFileInfo
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = pSystemFileInfo->m_Qualifier;
    fileSpecification.m_FileName = pSystemFileInfo->m_Filename;
    fileSpecification.m_ReadKey = pSystemFileInfo->m_ReadKey;
    fileSpecification.m_WriteKey = pSystemFileInfo->m_WriteKey;
    FacilitiesManager::FieldList additionalFields;

    FacilitiesManager* pFacMgr = dynamic_cast<FacilitiesManager*>( m_pExec->getManager( Exec::MID_FACILITIES_MANAGER ) );
    FacilitiesManager::Result result = pFacMgr->asg( this,
                                                     m_pExec->getRunInfo()->getSecurityContext(),
                                                     m_pExec->getRunInfo(),
                                                     pSystemFileInfo->m_AsgOptions,
                                                     fileSpecification,
                                                     additionalFields );

    std::string msg = "Cannot @ASG " + pSystemFileInfo->m_Qualifier + "*" + pSystemFileInfo->m_Filename;
    return checkFacStatus( result, msg );
}


//  catalogSystemFile()
//
//  Catalogs one system file, based on the current state and the settings for the
//  SystemFileInfo object associated with that state.
/*TODO:RECOV
    //  For GENF$, If this isn't a JK13 (either a JK9 boot, or the file was destroyed somehow), notify the console
    if ( (sfid == SFID_GENF) && (!pObject->m_ScratchPad.m_JK13) )
        m_pConsoleManager->postReadOnlyMessage( "GENF$ RE-CATALOGED. NO RUNS OR PRINT RECOVERED",
                                                            Routing,
                                                            m_pExec->getRunInfo() );
*/
bool
BootActivity::catalogSystemFile
(
    SystemFileInfo* const   pSystemFileInfo
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = pSystemFileInfo->m_Qualifier;
    fileSpecification.m_FileName = pSystemFileInfo->m_Filename;
    fileSpecification.m_ReadKey = pSystemFileInfo->m_ReadKey;
    fileSpecification.m_WriteKey = pSystemFileInfo->m_WriteKey;

    std::stringstream reserveStrm;
    reserveStrm << std::dec << pSystemFileInfo->m_InitialReserve;
    std::stringstream maxStrm;
    maxStrm << std::dec << pSystemFileInfo->m_MaximumSize;

    FacilitiesManager::Field field;
    field.push_back( FacilitiesManager::SubField( pSystemFileInfo->m_EquipmentType ) );
    field.push_back( FacilitiesManager::SubField( reserveStrm.str() ) );
    field.push_back( FacilitiesManager::SubField( "TRK" ) );
    field.push_back( FacilitiesManager::SubField( maxStrm.str() ) );
    FacilitiesManager::FieldList additionalFields;
    additionalFields.push_back( field );

    FacilitiesManager* pFacMgr = dynamic_cast<FacilitiesManager*>( m_pExec->getManager( Exec::MID_FACILITIES_MANAGER ) );
    FacilitiesManager::Result result = pFacMgr->cat( this,
                                                     m_pExec->getRunInfo()->getSecurityContext(),
                                                     m_pExec->getRunInfo(),
                                                     pSystemFileInfo->m_CatOptions,
                                                     fileSpecification,
                                                     additionalFields );

    std::string msg = "Cannot @CAT " + pSystemFileInfo->m_Qualifier + "*" + pSystemFileInfo->m_Filename;
    return checkFacStatus( result, msg );
}


//  checkFacStatus()
//
//  Checks the facilities status in the given result object, and if not successful
//  a console message is generated, and the exec is stopped.
inline bool
BootActivity::checkFacStatus
(
    const FacilitiesManager::Result&    result,
    const std::string&                  message
)
{
    if ( (result.m_StatusBitMask.getW() & 0400000000000ll) != 0 )
    {
        std::stringstream strm;
        strm << message << " Fac Status " << std::oct << result.m_StatusBitMask.getW();
        SystemLog::getInstance()->write( strm.str() );
        m_pConsoleManager->postReadOnlyMessage( strm.str(), Routing, m_pExec->getRunInfo() );
        m_pExec->stopExec( Exec::SC_INITIALIZATION_ASG_FAILED );
        return false;
    }

    return true;
}


//  confirmFixedDeviceCount()
bool
BootActivity::confirmFixedDeviceCount()
{
    //  Prompt operator to ensure the proper number of fixed drives.
    //  Part of this process entails directing MFD to read the disk labels of the accessible packs.
    //      n-FIXED MS DEVICES= 1 - CONTINUE? YN
    COUNT fixedCount = 0;
    MFDManager* pMfdMgr = dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) );
    pMfdMgr->readDiskLabels( this, &fixedCount );
    if ( fixedCount == 0 )
    {
        //  Oops - there aren't any fixed packs.  We cannot proceed.
        std::string consMsg = "INVALID CONFIGURATION-NO FIXED DEVICES FOUND";
        m_pConsoleManager->postReadOnlyMessage( consMsg, Routing, m_pExec->getRunInfo() );
        m_pExec->stopExec( Exec::SC_INITIALIZATION );
        return false;
    }

    std::stringstream strm;
    strm << "FIXED MS DEVICES=" << std::setw( 2 ) << std::dec << fixedCount << " - CONTINUE? YN";
    VSTRING responses;
    responses.push_back( "Y" );
    responses.push_back( "N" );
    INDEX respIndex = 0;
    if ( !m_pConsoleManager->postReadReplyMessage( strm.str(),
                                                   responses,
                                                   &respIndex,
                                                   m_pExec->getRunInfo() ) )
    {
        return false;
    }

    if ( !isWorkerTerminating() && respIndex == 1 )
    {
        m_pExec->stopExec( Exec::SC_OPERATOR_KEYIN );
        return false;
    }

    return true;
}


//  confirmJK13()
//
//  Remind operator that JK13 is set, and prompt whether he really wants this.
//      JK 13 is set and will cause mass storage to be reinitialized,
//      0-are you sure you want JK 13 set? Answer Y or N
bool
BootActivity::confirmJK13()
{
    m_pConsoleManager->postReadOnlyMessage( "JK 13 is set and will cause mass storage to be reinitialized.",
                                            Routing,
                                            m_pExec->getRunInfo() );
    if ( isWorkerTerminating() )
        return false;

    VSTRING responses;
    responses.push_back( "Y" );
    responses.push_back( "N" );
    INDEX respIndex = 0;
    if ( !m_pConsoleManager->postReadReplyMessage( "Are you sure you want JK 13 set?  Answer Y or N",
                                                   responses,
                                                   &respIndex,
                                                   m_pExec->getRunInfo() ) )
    {
        return false;
    }

    if ( !isWorkerTerminating() && respIndex == 1 )
    {
        m_pExec->stopExec( Exec::SC_OPERATOR_KEYIN );
        return false;
    }

    return true;
}


//  displayJumpKeysSet()
//
//  Possibly multi-step process to show the operator which jump keys are set during boot
//  Canonical format is along the lines of:
//      Jump keys set during boot with IP0 clocking are:
//      04, 07, 13
//  We don't have any IP clocking, so we drop that bit.
void
BootActivity::displayJumpKeysSet()
{
    m_pConsoleManager->postReadOnlyMessage( "Jump keys set during boot are:", Routing, m_pExec->getRunInfo() );
    if ( isWorkerTerminating() )
        return;

    std::vector<bool> jumpKeys;
    m_pExec->getJumpKeys( &jumpKeys );
    std::stringstream jkStream;
    jkStream << "  ";
    bool foundAny = false;
    for ( INDEX jkx = 0; jkx < 36; ++jkx )
    {
        if ( isWorkerTerminating() )
            return;

        if ( jumpKeys[jkx] )
        {
            if ( jkStream.str().size() > 70 )
            {
                m_pConsoleManager->postReadOnlyMessage( jkStream.str(), Routing, m_pExec->getRunInfo() );
                jkStream.str("  ");
            }
            if ( jkStream.str().size() > 2 )
                jkStream << ", ";
            jkStream << std::setw( 2 ) << std::setfill( '0' ) << (jkx + 1);
            foundAny = true;
        }
    }

    if ( isWorkerTerminating() )
        return;

    if ( foundAny )
    {
        if ( jkStream.str().size() > 2 )
            m_pConsoleManager->postReadOnlyMessage( jkStream.str(), Routing, m_pExec->getRunInfo() );
    }
    else
        m_pConsoleManager->postReadOnlyMessage( "  NONE", Routing, m_pExec->getRunInfo() );
}


//  displaySessionNumber()
//
//  Display boot type and session number -- one of the following:
//      Operator initiated initial session 000
//      Operator initiated recovery session 000
//      Auto-recovery initiated recovery session 000
void
BootActivity::displaySessionNumber()
{
    std::stringstream strm;
    if ( m_pExec->isOperatorBoot() )
        strm << "Operator";
    else
        strm << "Auto-recovery";
    strm << "-initiated ";
    if ( m_pExec->isInitialBoot() )
        strm << "initial";
    else
        strm << "recovery";
    strm << " session " << std::oct << std::setw( 3 ) << std::setfill( '0' ) << m_pExec->getCurrentSession();
    m_pConsoleManager->postReadOnlyMessage( strm.str(), Routing, m_pExec->getRunInfo() );
}


//  displayStartupMessages()
//
//  Display startup messages - Normally, these would include:
//      EXEC mass storage device will be D00027
//      1100 Operating System MP lev: 45.17H12*HG
//  We don't have an EXEC mass storage device, so we just do some identification stuff.
void
BootActivity::displayStartupMessages()
{
    std::string consMsg = "emExec System Console - " VERSION;
    m_pConsoleManager->postReadOnlyMessage( consMsg, Routing, m_pExec->getRunInfo() );
    m_pConsoleManager->postReadOnlyMessage( COPYRIGHT, Routing, m_pExec->getRunInfo() );
}


//  initialBoot()
//
//  JK13 boot path
void
BootActivity::initialBoot()
{
    if ( !confirmJK13() || isWorkerTerminating() )
        return;

    m_pExec->displayTime( m_pExec->getExecTime(), true, Routing );
    if ( isWorkerTerminating() )
        return;

    if ( !confirmFixedDeviceCount() || isWorkerTerminating() )
        return;

    waitForDownPackKeyins();
    if ( isWorkerTerminating() )
        return;

    if ( !initializeMassStorage() || isWorkerTerminating() )
        return;

    if ( !initializeGenf() || isWorkerTerminating() )
        return;

    if ( !initializeDloc() || isWorkerTerminating() )
        return;

    AccountManager* pAccMgr = dynamic_cast<AccountManager*>( m_pExec->getManager( Exec::MID_ACCOUNT_MANAGER ) );
    if ( !pAccMgr->initialize( this ) || isWorkerTerminating() )
        return;

    SecurityManager* pSecMgr = dynamic_cast<SecurityManager*>( m_pExec->getManager( Exec::MID_SECURITY_MANAGER ) );
    if ( !pSecMgr->initialize( this ) || isWorkerTerminating() )
        return;

    //  Start up some other activities
    CoarseSchedulerActivity* pCoarseSchedulerActivity = new CoarseSchedulerActivity( m_pExec );
    m_pExec->getRunInfo()->appendTaskActivity( pCoarseSchedulerActivity );
    pCoarseSchedulerActivity->start();

    PollActivity* pPollActivity = new PollActivity( m_pExec );
    m_pExec->getRunInfo()->appendTaskActivity( pPollActivity );
    pPollActivity->start();

    //  Create and start SYS, and wait until it is done (need intrinsic @LIBCOPY routine of some sort)
    m_pExec->setStatus( Exec::ST_SYS );
/*
SYS START
SYS SYS*MSG: CREATING - SYSTEM FILES
SYS SYS*MSG: CREATED - SYSTEM FILES
SYS SYS FIN
*/

/*
Main storage size = 264830976 words
*/

    m_pExec->setStatus( Exec::ST_RUNNING );

    //  Ready for RSI to get going...
    RSIManager* pRsiMgr = dynamic_cast<RSIManager*>( m_pExec->getManager( Exec::MID_RSI_MANAGER ) );
    RSIActivity* pRSIActivity = new RSIActivity( m_pExec, pRsiMgr );
    m_pExec->getRunInfo()->appendTaskActivity( pRSIActivity );
    pRSIActivity->start();

    //  And we're out of here...
}


//  initializeDloc()
//
//  Catalog SYS$*DLOC$
bool
BootActivity::initializeDloc()
{
    SystemFileInfo sfInfo( "SYS$", "DLOC$", "RDKDLC", "WRKDLC", 0, OPTB_G | OPTB_P | OPTB_V, m_DlocAssignMnemonic );
    return catalogSystemFile( &sfInfo );
}


//  initializeGenf()
//
//  Catalog and Assign SYS$*GENF$
bool
BootActivity::initializeGenf()
{
    SystemFileInfo sfInfo( "SYS$", "GENF$", "", "", OPTB_A | OPTB_X, OPTB_G | OPTB_V, m_GenfAssignMnemonic, m_GenfInitialReserve );
    if ( !catalogSystemFile( &sfInfo ) )
        return false;
    return assignSystemFile( &sfInfo );
}


//  initializeMassStorage()
//
//  Initialize mass storage.  Note how long this takes, then display the time elapsed.
//      MASS STORAGE INITIALIZED 704 MS.
bool
BootActivity::initializeMassStorage()
{
    COUNT64 startMicros = SystemTime::getMicrosecondsSinceEpoch();
    MFDManager* pMfdMgr = dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) );
    MFDManager::Result result = pMfdMgr->initialize( this );
    if ( result.m_Status == MFDManager::MFDST_OUT_OF_SPACE )
    {
        SystemLog::write("BootActivity:initializeMassStorage():MFDManager reporting MFDST_OUT_OF_SPACE");
        m_pExec->stopExec( Exec::SC_MASS_STORAGE_FULL );
        return false;
    }
    else if ( result.m_Status != MFDManager::MFDST_SUCCESSFUL )
    {
        std::string logMsg = "BootActivity::initializeMassStorage():";
        logMsg += MFDManager::getResultString( result );
        SystemLog::write(logMsg);
        m_pExec->stopExec( Exec::SC_DIRECTORY_ERROR );
        return false;
    }

    COUNT64 endMicros = SystemTime::getMicrosecondsSinceEpoch();
    std::stringstream strm;
    strm.str( "" );
    strm << "MASS STORAGE INITIALIZED " << (endMicros - startMicros) / 1000 << " MS.";
    m_pConsoleManager->postReadOnlyMessage( strm.str(), 0 );

    return true;
}


//  recoveryBoot()
//
//  Non-JK13 boot path
void
BootActivity::recoveryBoot()
{
    //TODO:RECOV
}


//  waitForDownPackKeyins()
//
//  Allow the user to mark packs down.  (We don't do this right now, but one day we might).
//      n-ENTER LOCAL DOWN PACK KEYINS - ANS GO
void
BootActivity::waitForDownPackKeyins()
{
    VSTRING responses;
    responses.push_back( "GO" );
    INDEX respIndex = 0;
    m_pConsoleManager->postReadReplyMessage( "ENTER LOCAL DOWN PACK KEYINS - ANS GO",
                                             responses,
                                             &respIndex,
                                             m_pExec->getRunInfo() );
    return;
}


//  waitForModifyConfig()
//
//  Allow the user to mark packs down.  (We don't do this right now, but one day we might).
//      n-Modify config, then answer: DONE
void
BootActivity::waitForModifyConfig()
{
    VSTRING responses;
    responses.push_back( "DONE" );
    INDEX respIndex = 0;
    m_pConsoleManager->postReadReplyMessage( "Modify config, then answer: DONE",
                                             responses,
                                             &respIndex,
                                             m_pExec->getRunInfo() );
}


//  worker()
//
//  Main code for this activity
void
BootActivity::worker()
{
    std::stringstream strm;

    //  Set boot status 1 - allow FS keyins, SJ/CJ/DJ, and D keyin to set time/date.
    m_pExec->setStatus( Exec::ST_BOOTING_1 );

    //  Set holds on batch and demand
    m_pExec->setHoldBatchRuns( true );
    m_pExec->setHoldDemandTerminals( true );

    displayStartupMessages();
    if ( isWorkerTerminating() )
        return;

    displayJumpKeysSet();
    if ( isWorkerTerminating() )
        return;

    //  If this an automatic boot (non-operator), display the system error message.
    //      System error ooo terminated session ooo.
    if ( !m_pExec->isOperatorBoot() )

    {
        m_pExec->displaySystemErrorMsg( m_pExec->getLastStopCode(),
                                        m_pExec->getCurrentSession() - 1 );
        if ( isWorkerTerminating() )
            return;
    }

    //  Canonically, modify config is allowed for tape boots.
    //  We don't have tape boots, but we do have the concept of reloading the OS...
    //  This is done by emexec, and results in discarding and reinstantiating the Exec object (and its dependencies).
    //  For our purposes, we'll just assume that all operator (manual) boots should ask the question.
    if ( m_pExec->isOperatorBoot() )
    {
        waitForModifyConfig();
        if ( isWorkerTerminating() )
            return;
    }

    //  Set boot status 1 - no more config changes for the time being...
    m_pExec->setStatus( Exec::ST_BOOTING_2 );

    displaySessionNumber();
    if ( isWorkerTerminating() )
        return;

    //  Branch here for jump 13.
    if ( m_pExec->getJumpKey( 13 ) )
        initialBoot();
    else
        recoveryBoot();
}


#if 0 //TODO:RECOV
//  displayJK9Alert()
//
//  If JK9 is set, we notify the operator and default to the next state for confirmation.
//  Otherwise, we skip to displaying date and time.
//
//  Only call if JK13 is NOT set.
void
    BootActivity::displayJK9Alert
    (
    IntrinsicActivity* const    pIntrinsic
    )
{
    BootActivity* pObject = dynamic_cast<BootActivity*>( pIntrinsic );
    if ( pObject->m_ScratchPad.m_JK9 )
        m_pConsoleManager->postReadOnlyMessage( "JK 9 is set to query for re-catalog of system files.", Routing );
    else
        pObject->setNextState( DISPLAY_DATE_AND_TIME );
}
#endif



// constructors / destructors

BootActivity::BootActivity
(
    Exec* const         pExec
)
:IntrinsicActivity( pExec, "BootActivity", pExec->getRunInfo() )
{
    m_pConsoleManager = dynamic_cast<ConsoleManager*>( pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );

    //  Load config parameters
    m_DefaultDiskType = m_pExec->getConfiguration().getStringValue( "MDFALT" );
    m_DlocAssignMnemonic = m_pExec->getConfiguration().getStringValue( "DLOCASGMNE" );
    m_GenfAssignMnemonic = m_pExec->getConfiguration().getStringValue( "GENFASGMNE" );
    m_GenfInitialReserve = m_pExec->getConfiguration().getIntegerValue( "GENFINTRES" );
    m_LibAssignMnemonic = m_pExec->getConfiguration().getStringValue( "LIBASGMNE" );
    m_LibInitialReserve = m_pExec->getConfiguration().getIntegerValue( "LIBINTRES" );
    m_LibMaxSize = m_pExec->getConfiguration().getIntegerValue( "LIBMAXSIZ" );
    m_RunAssignMnemonic = m_pExec->getConfiguration().getStringValue( "RUNASGMNE" );
    m_RunInitialReserve = m_pExec->getConfiguration().getIntegerValue( "RUNINTRES" );
    m_RunMaxSize = m_pExec->getConfiguration().getIntegerValue( "RUNMAXSIZ" );
}



//	public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
    BootActivity::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "BootActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}


