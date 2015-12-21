//  SSPPanel.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implements a PanelInterface for the SSP



#ifndef SSPPANEL_H
#define	SSPPANEL_H



class   SSPPanel : public PanelInterface, public Worker, public Lockable
{
private:
    ConsoleInterface* const     m_pConsole;
    Exec*                       m_pExec;
    Word36                      m_JumpKeys;
    bool                        m_RestartEnabled;
    bool                        m_RestartRequested;
    std::string                 m_StatusMessage;
    Exec::StopCode              m_StopCode;
    std::string                 m_StopCodeMessage;

    static UINT64       getJumpKeyMask( const JUMPKEY jumpKey );
    static UINT64       getNotJumpKeyMask( const JUMPKEY jumpKey );

    void                worker();

public:
    SSPPanel( ConsoleInterface* const pConsole )
    :Worker( "SSPPanel" ),
    m_pConsole( pConsole )
    {
        m_pExec = 0;
        m_RestartEnabled = false;
        m_RestartRequested = false;
        m_StopCode = static_cast<Exec::StopCode>( 0 );
        workerStart();
    }

    virtual ~SSPPanel()
    {
        workerSetTermFlag();
    }

    bool                    bootExec( const bool operatorBoot );
    bool                    dumpExec() const;
    bool                    haltExec() const;
    bool                    loadExec( Configuration* const  pConfiguration,
                                      NodeTable* const      pNodeTable );
    bool                    reloadExec( Configuration* const    pConfiguration,
                                        NodeTable* const        pNodeTable );
    bool                    unloadExec();

    inline std::string      getStatusMessage() const    { return m_StatusMessage; }
    inline Exec::StopCode   getStopCode() const         { return m_StopCode; }
    inline std::string      getStopCodeMessage() const  { return m_StopCodeMessage; }
    inline bool             isExecLoaded() const        { return m_pExec != 0; }
    inline bool             isExecRunning() const       { return (m_pExec != 0) && m_pExec->isRunning(); }

    //  PanelInterface overrides
    bool                    isJumpKeySet( const JUMPKEY jumpKey ) const;
    bool                    getJumpKey( const JUMPKEY jumpKey ) const;
    Word36                  getJumpKeys() const;
    void                    setJumpKey( const JUMPKEY   jumpKey,
                                        const bool      value );
    void                    setJumpKeys( const Word36& jumpKeys );
    void                    setStatusMessage( const std::string& message );
    void                    setStopCodeMessage( const std::string& message );
};



#endif	/* SSPPANEL_H */

