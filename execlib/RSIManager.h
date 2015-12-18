//	RSIManager.h
//
//  Handles RSI sessions



#ifndef		EXECLIB_RSI_MANAGER_H
#define		EXECLIB_RSI_MANAGER_H



#include    "ControlModeRunInfo.h"
#include    "ExecManager.h"
#include    "RSIActivity.h"
#include    "RSISession.h"



class RSIManager : public ExecManager
{
public:
    enum Status
    {
        ST_SUCCESSFUL,
        ST_NOT_READY,           //  for output, we have too much queued up already; for input, nothing is available
        ST_NO_DEVICE,           //  no such session, or session is terminating - probably, the session was closed
    };

private:
    typedef std::map<COUNT, RSISession*>            SESSIONS;
    typedef SESSIONS::iterator                      ITSESSIONS;
    typedef SESSIONS::const_iterator                CITSESSIONS;

    Activity*                       m_pActivity;                //  pointer to RSIActivity
    COUNT                           m_ConfigMaxSignOnAttempts;
    COUNT                           m_ConfigMaxSessions;
    SESSIONS                        m_Sessions;

    /* obsolete????
    void                            executeCM( RSISession* const       pSession,
                                                const std::string&  message ) const;
    void                            executeTERM( RSISession* const pSession ) const;
    */
    bool                            generateSessionIds( COUNT* const        pSessionNumber,
                                                        std::string* const  pSessionName );
    bool                            generateSessionNumber( COUNT* const pSessionNumber );
    bool                            handleSessionActive( RSISession* const pSession );
    bool                            handleSessionConnected( RSISession* const pSession );
    bool                            handleSessionInactive( RSISession* const pSession );
    bool                            handleSessionLoginAccepted( RSISession* const pSession );
    bool                            handleSessionLoginSolicited( RSISession* const pSession );
    bool                            handleSessionSolicitLogin( RSISession* const pSession );
    bool                            handleSessionTerminate( RSISession* const pSession );
    bool                            handleSessionTerminateDelete( RSISession* const pSession );
    bool                            handleSessionTerminateWaitAct( RSISession* const pSession );
    bool                            handleSessionTerminateWaitRunInfo( RSISession* const pSession );
    bool                            handleSessionWaitingForRunImage( RSISession* const pSession );

    static bool                     checkAndExecuteTransparentImage( RSISession* const         pSession,
                                                                        const std::string&  image );
    static void                     checkPrintBuffer( RSISession* const pSession );
    static void                     parseUserIdPassword( RSISession* const  pSession,
                                                         const SuperString& input,
                                                         bool* const        pBypassRunImage,
                                                         SuperString* const pUserId,
                                                         SuperString* const pPassword,
                                                         SuperString* const pNewPassword );
    static bool                     timeoutCheck( RSISession* const pSession );
    static void                     timeoutReset( RSISession* const pSession );


public:
	RSIManager( Exec* const pExec );

    Status                          getUserId( const COUNT          sessionNumber,
                                               std::string* const   pUserId ) const;
//????    bool                            openSession( SmartConsole* const        pScreen,//  TODO needs to be a real UTS screen...
//                                                     const std::string* const   pSessionName = 0 );
    Status                          sendOutput( const std::string&  text,
                                                const bool          force = false );    //  Send to all terminals (i.e., TB keyin)
    Status                          sendOutput( const COUNT         sessionNumber,
                                                const std::string&  text,
                                                const bool          force = false );    //TODO force overrides @@HOLD... when we get that far
    Status                          terminateSession( const COUNT sessionNumber );
    bool                            poll();

    //  inlines
    void                            setActivity( RSIActivity* const pActivity )     { m_pActivity = pActivity; }

	//  ExecManager interface
    void                            cleanup();
    void                            dump( std::ostream&     stream,
                                          const DUMPBITS    dumpBits );
	void							shutdown();
	bool							startup();
    void                            terminate();
};



#endif  // EXECLIB_RSI_MANAGER_H
