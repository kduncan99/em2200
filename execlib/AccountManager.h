//  AccountManager.h header file
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EMEXEC_ACCOUNT_MANAGER_H
#define     EMEXEC_ACCOUNT_MANAGER_H



#include    "Activity.h"
#include    "ExecManager.h"



class   AccountManager : public ExecManager
{
private:
    //  config values
    SuperString             m_AcctAssignMnemonic;       //  ACCTASGMNE  - asg mnemonic for SYS$*ACCOUNT$R1 and SYS$*SEC@ACCTINFO
    COUNT                   m_AcctInitialReserve;       //  ACCTINTRES  - initial reserve for SYS$*ACCOUNT$R1 and SYS$*SEC@ACCTINFO

    bool                    assignAccountFile( Activity* const pBootActivity );
    bool                    catalogAccountFile( Activity* const pBootActivity );
    void                    getConfigData();
    void                    reportError();

public:
    AccountManager( Exec* const pExec )
        :ExecManager( pExec )
    {}

    bool                    initialize( Activity* const pBootActivity );
    bool                    recover( Activity* const pBootActivity );
    bool                    verifyAccount( const std::string& accountId ) const;

	//  ExecManager interface
    void                    cleanup();
    void                    dump( std::ostream&     stream,
                                  const DUMPBITS    dumpBits );
	void                    shutdown();
	bool                    startup();
    void                    terminate();
};



#endif
