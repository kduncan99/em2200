//  ExecManager base class implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  private, protected methods

AccountManager*
ExecManager::getAccountManager() const
{
    return dynamic_cast<AccountManager*>(m_pExec->getManager( Exec::MID_ACCOUNT_MANAGER ));
}


ConsoleManager*
ExecManager::getConsoleManager() const
{
    return dynamic_cast<ConsoleManager*>(m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ));
}


DeviceManager*
ExecManager::getDeviceManager() const
{
    return dynamic_cast<DeviceManager*>(m_pExec->getManager( Exec::MID_DEVICE_MANAGER ));
}


FacilitiesManager*
ExecManager::getFacilitiesManager() const
{
    return dynamic_cast<FacilitiesManager*>(m_pExec->getManager( Exec::MID_FACILITIES_MANAGER ));
}


IoManager*
ExecManager::getIoManager() const
{
    return dynamic_cast<IoManager*>(m_pExec->getManager( Exec::MID_IO_MANAGER ));
}


MFDManager*
ExecManager::getMFDManager() const
{
    return dynamic_cast<MFDManager*>(m_pExec->getManager( Exec::MID_MFD_MANAGER ));
}


QueueManager*
ExecManager::getQueueManager() const
{
    return dynamic_cast<QueueManager*>(m_pExec->getManager( Exec::MID_QUEUE_MANAGER ));
}


RSIManager*
ExecManager::getRSIManager() const
{
    return dynamic_cast<RSIManager*>(m_pExec->getManager( Exec::MID_RSI_MANAGER ));
}


SecurityManager*
ExecManager::getSecurityManager() const
{
    return dynamic_cast<SecurityManager*>(m_pExec->getManager( Exec::MID_SECURITY_MANAGER ));
}



//  construtors, destructors



//  public (default) methods

//  cleanup()
//
//  Default handler called just before Exec is deleted
void
ExecManager::cleanup()
{
}


//  dump()
//
//  For debugging
void
ExecManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
}


//  shutdown()
//
//  Default handler called early in stopExec...
void
ExecManager::shutdown()
{
}


//  startup()
//
//  Default handler called early in boot...
bool
ExecManager::startup()
{
    return true;
}


//  terminate()
//
//  Default handler called late in stopExec...
void
ExecManager::terminate()
{
}

