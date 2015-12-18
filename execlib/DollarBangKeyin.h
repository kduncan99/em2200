//  DollarBangKeyin.h
//
//  $! keyin handler



#ifndef     EXECLIB_DOLLAR_BANG_KEYIN_H
#define     EXECLIB_DOLLAR_BANG_KEYIN_H



#include    "Exec.h"
#include    "KeyinActivity.h"



class	DollarBangKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void                        handler();
    bool                        isAllowed() const;

public:
    DollarBangKeyin( Exec* const                        pExec,
                     const SuperString&                 KeyinId,
                     const SuperString&                 Option,
                     const std::vector<SuperString>&    Params,
                     const Word36&                      Routing );
};



#endif

