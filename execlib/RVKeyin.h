//	RVKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	RV keyin handler



#ifndef		EXECLIB_RV_KEYIN_H
#define		EXECLIB_RV_KEYIN_H



#include    "Exec.h"
#include	"FacilitiesKeyin.h"



class	RVKeyin : public FacilitiesKeyin
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    RVKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

