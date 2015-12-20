//	CJKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	CJ keyin handler



#ifndef		EXECLIB_CJ_KEYIN_H
#define		EXECLIB_CJ_KEYIN_H



#include    "Exec.h"
#include    "JumpKeyKeyin.h"



class   CJKeyin : public JumpKeyKeyin
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    CJKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

