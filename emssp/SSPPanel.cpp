//  Implementation of a PanelInterface for the SSP



#include    "emssp.h"



//  ----------------------------------------------------------------------------
//  Private static methods
//  ----------------------------------------------------------------------------

UINT64
SSPPanel::getJumpKeyMask
(
    const JUMPKEY   jumpKey
)
{
    return 0400000000000LL >> (jumpKey - 1);
}


UINT64
SSPPanel::getNotJumpKeyMask
(
    const JUMPKEY   jumpKey
)
{
    return getJumpKeyMask( jumpKey ) ^ 0777777777777LL;
}


//  worker()
//
//  Responsible for rebooting an exec if appropriate
void
SSPPanel::worker()
{
    while ( !isWorkerTerminating() )
    {
        lock();

        if ( m_pExec && !m_pExec->isRunning() && m_RestartRequested )
        {
            m_RestartRequested = false;
            m_pExec->bootExec( false );
        }

        unlock();
        miscSleep( 1000 );
    }
}


//  ----------------------------------------------------------------------------
//  Public methods
//  ----------------------------------------------------------------------------

bool
SSPPanel::bootExec
(
    const bool      operatorBoot
)
{
    lock();

    if ( m_pExec == 0 )
    {
        unlock();
        std::cout << "Exec is not loaded" << std::endl;
        return false;
    }

    if ( m_pExec->isRunning() )
    {
        unlock();
        std::cout << "Exec is already running" << std::endl;
        return false;
    }

    if ( m_pExec->bootExec( operatorBoot ) )
        m_RestartEnabled = true;

    std::cout << "Exec booted" << std::endl;
    unlock();
    return true;
}


bool
SSPPanel::dumpExec() const
{
    lock();

    if ( m_pExec == 0 )
    {
        unlock();
        std::cout << "Exec is not loaded" << std::endl;
        return false;
    }

    std::string fileName = m_pExec->dump( 0xFFFFFFFF );

    unlock();
    std::cout << "Dump complete - File:" << fileName << std::endl;
    return true;
}


bool
SSPPanel::haltExec() const
{
    lock();

    if ( m_pExec == 0 )
    {
        unlock();
        std::cout << "Exec is not loaded" << std::endl;
        return false;
    }

    if ( !m_pExec->isRunning() )
    {
        unlock();
        std::cout << "Exec is not running" << std::endl;
        return false;
    }

    m_pExec->stopExec( Exec::SC_PANEL_HALT );
    unlock();
    std::cout << "Stop signal sent to Exec" << std::endl;
    return true;
}


bool
SSPPanel::loadExec
(
    Configuration* const    pConfiguration,
    NodeTable* const        pNodeTable
)
{
    lock();
    if ( m_pExec != 0 )
    {
        unlock();
        std::cout << "Exec is already loaded" << std::endl;
        return false;
    }

    m_pExec = new Exec( this, pConfiguration, pNodeTable );
    m_RestartEnabled = false;
    m_RestartRequested = false;
    unlock();

    m_pExec->registerConsole( m_pConsole, true );
    std::cout << "Exec loaded" << std::endl;
    return true;
}


bool
SSPPanel::reloadExec
(
    Configuration* const    pConfiguration,
    NodeTable* const        pNodeTable
)
{
    lock();
    if ( m_pExec == 0 )
    {
        unlock();
        std::cout << "Exec is not loaded" << std::endl;
        return false;
    }

    if ( m_pExec->isRunning() )
    {
        unlock();
        std::cout << "Exec is running" << std::endl;
        return false;
    }

    delete m_pExec;
    m_pExec = new Exec( this, pConfiguration, pNodeTable );
    m_RestartEnabled = false;
    m_RestartRequested = false;
    unlock();

    m_pExec->registerConsole( m_pConsole, true );
    std::cout << "Exec has been reloaded" << std::endl;
    return true;
}


bool
SSPPanel::unloadExec()
{
    lock();
    if ( m_pExec == 0 )
    {
        unlock();
        std::cout << "Exec is not loaded - try LOAD" << std::endl;
        return false;
    }

    if ( m_pExec->isRunning() )
    {
        unlock();
        std::cout << "Exec is running - try HALT" << std::endl;
        return false;
    }

    delete m_pExec;
    m_pExec = 0;
    m_RestartEnabled = false;
    m_RestartRequested = false;
    unlock();
    std::cout << "Exec is unloaded" << std::endl;
    return true;
}


//  ----------------------------------------------------------------------------
//  PanelInterface overrides
//  ----------------------------------------------------------------------------

bool
SSPPanel::getJumpKey
(
    const JUMPKEY jumpKey
) const
{
    return ( m_JumpKeys.getValue() & getJumpKeyMask( jumpKey ) ) != 0;
}


Word36
SSPPanel::getJumpKeys() const
{
    return m_JumpKeys;
}


void
SSPPanel::setJumpKey
(
    const PanelInterface::JUMPKEY   jumpKey,
    const bool                      value
)
{
    if ( value )
    {
        m_JumpKeys.logicalOr( getJumpKeyMask( jumpKey ) );
    }
    else
    {
        m_JumpKeys.logicalAnd( getNotJumpKeyMask( jumpKey ) );
    }
}


void
SSPPanel::setJumpKeys
(
    const Word36& jumpKeys
)
{
    m_JumpKeys = jumpKeys;
}


void
SSPPanel::setStatusMessage
(
    const std::string&  message
)
{
    m_StatusMessage = message;
    std::cout << "EXST:" << message << std::endl;
}


void
SSPPanel::setStopCodeMessage
(
    const std::string&  message
)
{
    m_StopCodeMessage = message;
    m_StopCode = m_pExec->getLastStopCode();
    std::cout << "EXSC:" << message << std::endl;

    if ( m_RestartEnabled && !getJumpKey( 3 ) )
        m_RestartRequested = true;
}
