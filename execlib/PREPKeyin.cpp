//  PREPKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Handles the PREP keyin
//  This will format the disk (if appropriate) and write the label, then call MFD to initialize the pack.



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
PREPKeyin::isAllowed() const
{
	return true;
}


//	handler()
//
//	Called from Keyin base class worker() function
void
PREPKeyin::handler()
{
    //  Format is PREP,{type} {device},{label}
    //  Where {type} is blank or 'F' for fixed, or 'R' for removable
    //  device must be RV status

    //  Validate type.  For now, we only accept blank or 'F'.  Later, we will do removable.
    //  TODO:REM
    if ( (m_Option.size() != 0) && (m_Option.compareNoCase( "F" ) != 0) )
    {
        displayInvalidOption();
        return;
    }

    bool fixedFlag = true;

    //  Make sure we have the right number of parameters
    if ( m_Parameters.size() != 2 )
    {
        displayGeneralError();
        return;
    }

    //  Validate the device parameter
    SuperString deviceName = m_Parameters[0];
    deviceName.foldToUpperCase();
    if ( (deviceName.size() == 0) || (deviceName.size() > 6) )
    {
        displayInvalidParameter();
        return;
    }

    std::string consMsg = m_KeyinId + " " + deviceName + ":";

    DeviceManager* pdevmgr = dynamic_cast<DeviceManager*>( m_pExec->getManager( Exec::MID_DEVICE_MANAGER ) );
    const DeviceManager::DeviceEntry* pDeviceEntry = pdevmgr->getDeviceEntry( deviceName );
    if ( (pDeviceEntry == 0)
          || (reinterpret_cast<const Device *>( pDeviceEntry->m_pNode )->getDeviceType() != Device::DeviceType::DISK) )
    {
        consMsg += "IS NOT A CONFIGURED DISK";
        m_pConsoleManager->postReadOnlyMessage( consMsg, m_Routing, m_pExec->getRunInfo() );
        return;
    }

    //  Validate label
    SuperString packName = m_Parameters[1];
    packName.foldToUpperCase();
    if ( !miscIsValidPackName( packName ) )
    {
        consMsg += "INVALID PACK LABEL";
        m_pConsoleManager->postReadOnlyMessage( consMsg, m_Routing, m_pExec->getRunInfo() );
        return;
    }

    DeviceManager::NODE_ID nodeId = pDeviceEntry->m_NodeId;
    MFDManager* pmfdmgr = dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) );
    MFDManager::Result result = pmfdmgr->prepDisk( this, nodeId, fixedFlag, packName );
    if ( result.m_Status != MFDManager::MFDST_SUCCESSFUL )
    {
        consMsg += MFDManager::getResultString( result );
        m_pConsoleManager->postReadOnlyMessage( consMsg, m_Routing, m_pExec->getRunInfo() );
        return;
    }

    consMsg += "COMPLETE";
    m_pConsoleManager->postReadOnlyMessage( consMsg, m_Routing, m_pExec->getRunInfo() );
}



// constructors / destructors

PREPKeyin::PREPKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:KeyinActivity( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods

