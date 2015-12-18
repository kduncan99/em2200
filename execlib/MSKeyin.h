//	MSKeyin.h
//
//	MS keyin handler



#ifndef		EXECLIB_MS_KEYIN_H
#define		EXECLIB_MS_KEYIN_H



#include    "Exec.h"
#include	"KeyinActivity.h"



class	MSKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    MSKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

