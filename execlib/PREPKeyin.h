//  PREPKeyin.h
//
//  PREP keyin handler



#ifndef     EXECLIB_PREP_KEYIN_H
#define     EXECLIB_PREP_KEYIN_H



#include    "Exec.h"
#include    "KeyinActivity.h"



class   PREPKeyin : public KeyinActivity
{
private:
    // KeyinThread interface
    void                        handler();
    bool                        isAllowed() const;

public:
    PREPKeyin( Exec* const                      pExec,
               const SuperString&               KeyinId,
               const SuperString&               Option,
               const std::vector<SuperString>&  Params,
               const Word36&                    Routing );
};



#endif
