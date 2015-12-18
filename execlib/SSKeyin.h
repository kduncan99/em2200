//  SSKeyin.h
//
//  SS keyin handler



#ifndef     EXECLIB_SS_KEYIN_H
#define     EXECLIB_SS_KEYIN_H



#include    "Exec.h"
#include    "KeyinActivity.h"



class   SSKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    SSKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );

    static std::string          getStatusLine1();
    static std::string          getStatusLine2( Exec* const             pExec,
                                                ConsoleManager* const   pConsoleManager );
    static std::string          getStatusLine3( Exec* const pExec );
    static std::string          getStatusLine4();
};



#endif

