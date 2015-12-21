//	DJKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	DJ keyin handler



#ifndef     EXECLIB_DJ_KEYIN_H
#define     EXECLIB_DJ_KEYIN_H



#include    "Exec.h"
#include    "JumpKeyKeyin.h"



class   DJKeyin : public JumpKeyKeyin
{
private:
    // KeyinThread interface
    void                        handler();
    bool                        isAllowed() const;

public:
    DJKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

