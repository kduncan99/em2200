//	SUKeyin.h
//
//	SU keyin handler



#ifndef		EXECLIB_SU_KEYIN_H
#define		EXECLIB_SU_KEYIN_H



#include    "Exec.h"
#include	"FacilitiesKeyin.h"



class	SUKeyin : public FacilitiesKeyin
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    SUKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

