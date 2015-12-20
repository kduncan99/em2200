//	SecurityManager.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Implementation of SecurityManager class
//  Manages all security for emexec
//
//  The following files are created and assigned to the operating system:
//      SYS$*TSS$FILE - contains all user-id records with their attributes



#include	"execlib.h"



//	statics



//  private methods

//  assignSystemFile()
//
//  Assigns the TSS file (presuming it has been cataloged)
bool
SecurityManager::assignSecurityFile
(
    Activity* const     pBootActivity
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = "SYS$";
    fileSpecification.m_FileName = "TSS$FILE";
    fileSpecification.m_ReadKey = "";
    fileSpecification.m_WriteKey = "";

    FacilitiesManager::FieldList additionalFields;

    FacilitiesManager* pFacMgr = dynamic_cast<FacilitiesManager*>( m_pExec->getManager( Exec::MID_FACILITIES_MANAGER ) );
    FacilitiesManager::Result result = pFacMgr->asg( pBootActivity,
                                                     m_pExec->getRunInfo()->getSecurityContext(),
                                                     m_pExec->getRunInfo(),
                                                     OPTB_A | OPTB_X,
                                                     fileSpecification,
                                                     additionalFields );
    if ( (result.m_StatusBitMask.getW() & 0400000000000ll) != 0 )
        return false;

    return true;
}


//  catalogSecurityFile()
//
//  Catalogs the TSS file
bool
SecurityManager::catalogSecurityFile
(
    Activity* const     pBootActivity
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = "SYS$";
    fileSpecification.m_FileName = "TSS$FILE";
    fileSpecification.m_ReadKey = "";
    fileSpecification.m_WriteKey = "";

    std::stringstream reserveStrm;
    reserveStrm << std::dec << m_ConfigUserInitialReserve;

    FacilitiesManager::Field field;
    field.push_back( FacilitiesManager::SubField( m_ConfigUserAssignMnemonic ) );
    field.push_back( FacilitiesManager::SubField( reserveStrm.str() ) );
    field.push_back( FacilitiesManager::SubField( "TRK" ) );
    field.push_back( FacilitiesManager::SubField( "999" ) );
    FacilitiesManager::FieldList additionalFields;
    additionalFields.push_back( field );

    FacilitiesManager* pFacMgr = dynamic_cast<FacilitiesManager*>( m_pExec->getManager( Exec::MID_FACILITIES_MANAGER ) );
    FacilitiesManager::Result result = pFacMgr->cat( pBootActivity,
                                                     m_pExec->getRunInfo()->getSecurityContext(),
                                                     m_pExec->getRunInfo(),
                                                     OPTB_G | OPTB_V,
                                                     fileSpecification,
                                                     additionalFields );
    if ( (result.m_StatusBitMask.getW() & 0400000000000ll) != 0 )
        return false;

    return true;
}


//  getConfigData()
//
//  (re)load data items which derive from configurator entries
void
SecurityManager::getConfigData()
{
    m_ConfigSecurityOfficerUserId = m_pExec->getConfiguration().getStringValue( "SECOFFDEF" );
    m_ConfigUserAssignMnemonic = m_pExec->getConfiguration().getStringValue( "USERASGMNE" );
    m_ConfigUserInitialReserve = m_pExec->getConfiguration().getIntegerValue( "USERINTRES" );
}


//  getSecurityOfficerUserId()
//
//  If configured, we simply return the configured value.  If not, we solicit it.
SuperString
SecurityManager::getSecurityOfficerUserId
(
    Activity* const         pActivity
) const
{
    std::string userId = m_ConfigSecurityOfficerUserId;
    if ( userId.size() == 0 )
    {
        ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
        while ( !Exec::isValidUserId( userId ) )
        {
            ConsoleManager::ReadReplyRequest consRequest( pActivity->getRunInfo(),
                                                          0,
                                                          ConsoleManager::Group::SYSTEM,
                                                          "ENTER 12-CHARACTER SECURITY OFFICER USER-ID",
                                                          12 );
            pConsMgr->postReadReplyMessage( &consRequest, true );
            userId = consRequest.getResponse();
            pConsMgr->postReadOnlyMessage( "INVALID USER-ID SPECIFIED", 0 );
        }
    }

    return userId;
}



//  Constructors, destructors

SecurityManager::SecurityManager
(
    Exec* const         pExec
)
:ExecManager( pExec )
{
}



//  Public methods

//  cleanup()
void
SecurityManager::cleanup()
{
}


//  dump()
//
//  For debugging
void
SecurityManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "SecurityManager ----------" << std::endl;
}


//  initialize()
//
//  Called during boot to initialize security files
bool
SecurityManager::initialize
(
    Activity* const     pBootActivity
)
{
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );

    m_SecurityOfficerUserId = getSecurityOfficerUserId( pBootActivity );
    if ( pBootActivity->isTerminating() )
        return false;
    m_SecurityOfficerPassword = "SECURI";

    if ( !catalogSecurityFile( pBootActivity ) )
    {
        pConsMgr->postReadOnlyMessage( "TSS INITIALIZATION FAILURE", 0 );
        m_pExec->stopExec( Exec::SC_INITIALIZATION );
        return false;
    }

    if ( pBootActivity->isTerminating() )
        return false;

    if ( !assignSecurityFile( pBootActivity ) )
    {
        pConsMgr->postReadOnlyMessage( "TSS INITIALIZATION FAILURE", 0 );
        m_pExec->stopExec( Exec::SC_INITIALIZATION );
        return false;
    }

    if ( pBootActivity->isTerminating() )
        return false;

    //  Write security records
    //TODO:SEC

    //  Done
    pConsMgr->postReadOnlyMessage( "SECURITY INITIALIZATION COMPLETE", 0 );
    return true;
}


//  recover()
//
//  Called during boot to recover security files
bool
SecurityManager::recover
(
    Activity* const     pBootActivity
)
{
    //TODO:SEC
    return true;
}


//  shutdown()
//
//  Exec would like to shut down
void
SecurityManager::shutdown()
{
    SystemLog::write("SecurityManager::shutdown()");
}


//  startup()
//
//  Initializes the manager - Exec is booting
bool
SecurityManager::startup()
{
    SystemLog::write("SecurityManager::startup()");

    //  (re)load config data
    getConfigData();

    return true;
}


//  terminate()
//
//  Final termination of exec
void
SecurityManager::terminate()
{
    SystemLog::write("SecurityManager::terminate()");
}


//  userLoginFailure()
//
//  Notifies security system that a login failure occurred for the indicated user id
void
SecurityManager::userLoginFailure
(
    const SuperString&      userId
)
{
    //TODO:SEC do nothing for now
}


//  userLoginSuccess()
//
//  Notifies security system that a login successfully occurred for the indicated user id
void
SecurityManager::userLoginSuccess
(
    const SuperString&      userId
)
{
    //TODO:SEC do nothing for now
}


//  validateUser()
//
//  Validate the given userid with the given password and clearance level
SecurityManager::ValidationStatus
SecurityManager::validateUser
(
    const SuperString&      userId,
    const SuperString&      password,
    const SuperString&      newPassword,
    UserProfile* const      pProfile
)
{
    //TODO:SEC skeleton works - need fleshed out later
    if ( userId.compareNoCase( m_SecurityOfficerUserId ) != 0 )
        return VST_INCORRECT_USER_NAME;
    if ( password.compareNoCase( m_SecurityOfficerPassword ) != 0 )
        return VST_INCORRECT_PASSWORD;
    if ( !newPassword.empty() )
        m_SecurityOfficerPassword = newPassword;

    pProfile->m_AllAccountsAllowed = true;
    pProfile->m_AutomaticRunImage.clear();
    pProfile->m_CanAccessDemand = true;
    pProfile->m_CanAccessTIP = true;
    pProfile->m_CanBypassRunCard = true;
    pProfile->m_DefaultAccount = m_pExec->getConfiguration().getStringValue( "OVRACC" );
    pProfile->m_DemandTimeoutSeconds = 0;
    pProfile->m_Privileged = true;
    pProfile->m_UserId = m_SecurityOfficerUserId;

    if ( !newPassword.empty() )
        return VST_SUCCESSFUL_PASSWORD_CHANGED;
    else
        return VST_SUCCESSFUL;
}

