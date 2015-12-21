//	UPKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  UP keyin handler



#ifndef		EXECLIB_UP_KEYIN_H
#define		EXECLIB_UP_KEYIN_H



#include    "Exec.h"
#include	"FacilitiesKeyin.h"



class	UPKeyin : public FacilitiesKeyin
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    UPKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

