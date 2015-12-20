//  SecurityManager.h header file
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EMEXEC_SECURITY_MANAGER_H
#define     EMEXEC_SECURITY_MANAGER_H



#include    "Activity.h"
#include    "ExecManager.h"



class   SecurityManager : public ExecManager
{
private:
    //  config values
    SuperString             m_ConfigSecurityOfficerUserId;  //  SECOFFDEF - security officer ID for initial boot
    SuperString             m_ConfigUserAssignMnemonic;     //  USERASGMNE  - asg mnemonic for SYS$*SEC@USERID$
    COUNT                   m_ConfigUserInitialReserve;     //  USERINTRES  - initial reserve for SYS$*SEC@USERID$

    SuperString             m_SecurityOfficerPassword;      //  TODO - temporary while we're a skeleton
    SuperString             m_SecurityOfficerUserId;        //  TODO - temporary while we're a skeleton

    bool                    assignSecurityFile( Activity* const pBootActivity );
    bool                    catalogSecurityFile( Activity* const pBootActivity );
    void                    getConfigData();
    SuperString             getSecurityOfficerUserId( Activity* const pActivity ) const;


public:
    enum ValidationStatus
    {
        VST_SUCCESSFUL,
        VST_SUCCESSFUL_PASSWORD_CHANGED,
        VST_INCORRECT_PASSWORD,
        VST_INCORRECT_USER_NAME,
    };

    class UserProfile
    {
    public:
        bool                m_AllAccountsAllowed;
        VSTRING             m_AllowedAccounts;
        SuperString         m_AutomaticRunImage;
        bool                m_CanAccessBatch;
        bool                m_CanAccessDemand;
        bool                m_CanAccessTIP;
        bool                m_CanBypassRunCard;
        SuperString         m_DefaultAccount;
        COUNT               m_DemandTimeoutSeconds;     //  zero for infinite
        bool                m_Privileged;
        SuperString         m_UserId;
    };

    SecurityManager( Exec* const pExec );
    ~SecurityManager(){};

    bool                    initialize( Activity* const pBootActivity );
    bool                    recover( Activity* const pBootActivity );
    void                    userLoginFailure( const SuperString& userName );
    void                    userLoginSuccess( const SuperString& userName );
    ValidationStatus        validateUser( const SuperString&    userName,
                                          const SuperString&    password,
                                          const SuperString&    newPassword,
                                          UserProfile* const    pProfile );

	//  ExecManager interface
    void                    cleanup();
    void                    dump( std::ostream&     stream,
                                  const DUMPBITS    dumpBits );
	void                    shutdown();
	bool                    startup();
    void                    terminate();
};



#endif
