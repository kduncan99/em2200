//	CSKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	CS keyin handler



#ifndef		EXECLIB_CS_KEYIN_H
#define		EXECLIB_CS_KEYIN_H



#include    "Exec.h"
#include	"KeyinActivity.h"



class	CSKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void						handler();
    void                        handleA();
    void                        handleAD();
    void                        handleALL();
    void                        handleAT();
    void                        handleH();
    void                        handleHD();
    void                        handleHT();
    void                        handleRunid();
    bool						isAllowed() const;

public:
    CSKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

