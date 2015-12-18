//	DUKeyin.h
//
//	DU keyin handler



#ifndef     EXECLIB_DU_KEYIN_H
#define     EXECLIB_DU_KEYIN_H



#include    "Exec.h"
#include    "KeyinActivity.h"


class   DUKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void                        handler();
    bool                        isAllowed() const;

public:
    DUKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

