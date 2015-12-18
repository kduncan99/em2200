//  RSISession.h
//
//  Tracks a single RSI session for RSIManager



#ifndef     EXECLIB_RSI_SESSION_H
#define     EXECLIB_RSI_SESSION_H



#include    "DemandRunInfo.h"
#include    "SecurityManager.h"
#include    "TransparentActivity.h"



class RSISession : public Listener
{
public:
    enum State
    {
        STATE_CONNECTED,                //  Session established, but not active, and no login solicitation done
                                        //      transitions to STATE_LOGIN_SOLICITED
        STATE_SOLICIT_LOGIN,            //  Login needs to be solicited (may happen multiple times per STATE_CONNECTED)
        STATE_LOGIN_SOLICITED,          //  Login has been solicited
                                        //      transitions to STATE_LOGIN_ACCEPTED
        STATE_LOGIN_ACCEPTED,           //  Login has been accepted
                                        //      transitions to STATE_WAITING_FOR_RUN_IMAGE or
                                        //                      STATE_WAITING_FOR_RUN_IMAGE
        STATE_WAITING_FOR_RUN_IMAGE,    //  Credentials accepted, waiting for @RUN image
                                        //      transitions to STATE_ACTIVE
        STATE_ACTIVE,                   //  Session is active (thus, m_pRunInfo is not 0)
                                        //      transitions to STATE_TERMINATE
        STATE_TERMINATE,                //  Begin termination processing for the Session
                                        //      transitions to STATE_TERMINATE_WAIT_ACT or
                                        //                      STATE_TERMINATE_WAIT_RUNINFO or
                                        //                      STATE_INACTIVE
        STATE_TERMINATE_DELETE,         //  Session can finally be deleted
        STATE_TERMINATE_WAIT_ACT,       //  Session needs to terminate, but is waiting on @@ completion
                                        //      transitions to STATE_TERMINATE
        STATE_TERMINATE_WAIT_RUNINFO,   //  Session needs to terminate, but is waiting on the Exec to set RunInfo to ST_RSI_TERM
                                        //      transitions to STATE_TERMINATE
        STATE_INACTIVE,                 //  Session is inactive - it may go away, or it may be reactivated
                                        //      transitions to STATE_TERMINATE_DELETE or
                                        //                      STATE_CONNECTED
    };

private:
    bool                            m_BypassRunCard;        //  User wants to bypass automatic run card generation
    Exec* const                     m_pExec;                //  We need this here or there...
    COUNT64                         m_LastInputSystemTime;  //  system time of last input (non-offset)
    SecurityManager::UserProfile    m_LoginProfile;         //  User's profile at time of login - NOT kept up-to-date.
    DemandRunInfo*                  m_pRunInfo;
    const std::string               m_SessionName;
    const COUNT                     m_SessionNumber;
    State                           m_State;
    bool                            m_TimeoutFlag;          //  true if session is terminating due to timeout
    COUNT                           m_TimeoutSecs;          //  Current timeout value in seconds (0 for none)
    bool                            m_TimeoutWarning;       //  true indicating we've sent a warning message
    TransparentActivity*            m_pTransparentActivity; //  Some @@- command in progress
    SuperString                     m_UserId;               //  If credentials have been accepted
    COUNT                           m_UserIdErrorCount;     //  Number of consecutive user/password errors

public:
    RSISession( Exec* const         pExec,
                const COUNT         sessionNumber,
                const std::string&  sessionName );
    ~RSISession();

    void                            dump( std::ostream&             stream,
                                            const std::string&      prefix,
                                            const DUMPBITS          dumpBits );
    bool                            isTerminating() const;
    void                            registerTransparentActivity( TransparentCSInterpreter* const pInterpreter );
    void                            reset();
    void                            sendClearScreen() const;
    bool                            sendOutput( const std::string& text ) const;
    void                            unregisterTransparentActivity();

    //  inlines
    inline void                     incrementUserIdErrorCount()                     { ++m_UserIdErrorCount; }
//????    inline bool                     readInput( std::string* pInput ) const          { return m_pScreen->readInput( pInput ); }

    //  Listener interface //???? obsolete?
    void                            listenerEventTriggered( Event* const pEvent );

    //  getters
    inline bool                     getBypassRunCard() const                        { return m_BypassRunCard; }
    inline time_t                   getLastInputSystemTime() const                  { return m_LastInputSystemTime; }
    inline const SecurityManager::UserProfile&
                                    getLoginProfile() const                         { return m_LoginProfile; }
    inline DemandRunInfo*           getRunInfo() const                              { return m_pRunInfo; }
    inline const std::string&       getSessionName() const                          { return m_SessionName; }
    inline COUNT                    getSessionNumber() const                        { return m_SessionNumber; }
    inline State                    getState() const                                { return m_State; }
    inline bool                     getTimeoutFlag() const                          { return m_TimeoutFlag; }
    inline COUNT                    getTimeoutSecs() const                          { return m_TimeoutSecs; }
    inline bool                     getTimeoutWarning() const                       { return m_TimeoutWarning; }
    inline TransparentActivity*     getTransparentActivity() const                  { return m_pTransparentActivity; }
    inline const SuperString&       getUserId() const                               { return m_UserId; }
    inline COUNT                    getUserIdErrorCount() const                     { return m_UserIdErrorCount; }
//????    inline bool                     isScreenOpen() const                            { return m_pScreen->isOpen(); }

    //  setters
    inline void                     clearRunInfo()                                  { m_pRunInfo = 0; }
    inline void                     setBypassRunCard( const bool bypass )           { m_BypassRunCard = bypass; }
    inline void                     setLoginProfile( const SecurityManager::UserProfile& profile )
                                                                                    { m_LoginProfile = profile; }
    inline void                     setRunInfo( DemandRunInfo* pri )                { m_pRunInfo = pri; }
    inline void                     setState( const State state )                   { m_State = state; }
    inline void                     setTimeoutFlag( const bool flag )               { m_TimeoutFlag = flag; }
    inline void                     setTimeoutSecs( const COUNT seconds )           { m_TimeoutSecs = seconds; }
    inline void                     setTimeoutWarning( const bool flag )            { m_TimeoutWarning = flag; }
    inline void                     setUserId( const SuperString& userId )          { m_UserId = userId; }
    inline void                     updateLastInputTime()
    {
        m_LastInputSystemTime = SystemTime::getMicrosecondsSinceEpoch();
    }

    //  statics
    static std::string              getStateString( const State state );
};



#endif
