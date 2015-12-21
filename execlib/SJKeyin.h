//	SJKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	SJ keyin handler



#ifndef     EXECLIB_SJ_KEYIN_H
#define     EXECLIB_SJ_KEYIN_H



#include    "Exec.h"
#include    "JumpKeyKeyin.h"



class   SJKeyin : public JumpKeyKeyin
{
private:
    // KeyinThread interface
    void                        handler();
    bool                        isAllowed() const;

public:
    SJKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

