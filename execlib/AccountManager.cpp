//  AccountManager.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implementation of AccountManager class
//  Manages accounting for emexec
//
//  Catalogs and Assigns the following files to the operating system:
//      SYS$*ACCOUNT$R1



#include	"execlib.h"



//  private methods

//  assignSystemFile()
//
//  Assigns the TSS file (presuming it has been cataloged)
bool
AccountManager::assignAccountFile
(
    Activity* const     pBootActivity
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = "SYS$";
    fileSpecification.m_FileName = "ACCOUNT$R1";
    fileSpecification.m_ReadKey = "";
    fileSpecification.m_WriteKey = "";

    FacilitiesManager::FieldList additionalFields;

    FacilitiesManager::Result result = getFacilitiesManager()->asg( pBootActivity,
                                                                    m_pExec->getRunInfo()->getSecurityContext(),
                                                                    m_pExec->getRunInfo(),
                                                                    OPTB_A | OPTB_X,
                                                                    fileSpecification,
                                                                    additionalFields );
    if ( (result.m_StatusBitMask.getW() & 0400000000000ll) != 0 )
        return false;

    return true;
}


//  catalogAccountFile()
//
//  Catalogs the TSS file
bool
AccountManager::catalogAccountFile
(
    Activity* const     pBootActivity
)
{
    FileSpecification fileSpecification;
    fileSpecification.m_QualifierSpecified = true;
    fileSpecification.m_Qualifier = "SYS$";
    fileSpecification.m_FileName = "ACCOUNT$R1";
    fileSpecification.m_ReadKey = "";
    fileSpecification.m_WriteKey = "";

    std::stringstream reserveStrm;
    reserveStrm << std::dec << m_AcctInitialReserve;
    std::stringstream maxStrm;
    maxStrm << std::dec << 999;

    FacilitiesManager::Field field;
    field.push_back( FacilitiesManager::SubField( m_AcctAssignMnemonic ) );
    field.push_back( FacilitiesManager::SubField( reserveStrm.str() ) );
    field.push_back( FacilitiesManager::SubField( "TRK" ) );
    field.push_back( FacilitiesManager::SubField( maxStrm.str() ) );
    FacilitiesManager::FieldList additionalFields;
    additionalFields.push_back( field );

    FacilitiesManager::Result result = getFacilitiesManager()->cat( pBootActivity,
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
AccountManager::getConfigData()
{
    m_AcctAssignMnemonic = m_pExec->getConfiguration().getStringValue( "ACCTASGMNE" );
    m_AcctInitialReserve = m_pExec->getConfiguration().getIntegerValue( "ACCTINTRES" );
}


//  reportError()
void
AccountManager::reportError()
{
    ConsoleManager* pcmgr = getConsoleManager();
    pcmgr->postReadOnlyMessage( "An error occurred establishing the ACCOUNT$R1 file.", 0 );

    VSTRING responses;
    responses.push_back( "GO" );
    INDEX respIndex = 0;
    pcmgr->postReadReplyMessage( "Contact the site administrator. Answer GO.",
                                 responses,
                                 &respIndex,
                                 m_pExec->getRunInfo() );
    return;
}



//  Constructors, destructors



//  Public methods

//  cleanup()
void
AccountManager::cleanup()
{
}


//  dump()
//
//  For debugging
void
AccountManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "AccountManager ----------" << std::endl;
}


//  initialize()
//
//  Called during boot to initialize account files
bool
AccountManager::initialize
(
    Activity* const     pBootActivity
)
{
    if ( !catalogAccountFile( pBootActivity ) )
    {
        reportError();
        return false;
    }

    if ( pBootActivity->isTerminating() )
        return false;

    if ( !assignAccountFile( pBootActivity ) )
    {
        reportError();
        return false;
    }

    if ( pBootActivity->isTerminating() )
        return false;

    //  Write account records
    //TODO:ACCT

    //  Done
    return true;
}


//  recover()
//
//  Called during boot to recover security files
bool
AccountManager::recover
(
    Activity* const     pBootActivity
)
{
    //TODO:ACCT
    return true;
}


//  shutdown()
//
//  Exec would like to shut down
void
AccountManager::shutdown()
{
    SystemLog::write("AccountManager::shutdown()");
}


//  startup()
//
//  Initializes the manager - Exec is booting
bool
AccountManager::startup()
{
    SystemLog::write("AccountManager::startup()");

    //  (re)load config data
    getConfigData();

    return true;
}


//  terminate()
//
//  Final termination of exec
void
AccountManager::terminate()
{
    SystemLog::write("AccountManager::terminate()");
}


//  verifyAccount()
//
//  Ensures the account exists
bool
AccountManager::verifyAccount
(
    const std::string&      accountId
) const
{
    return true;//TODO:ACCT
}

