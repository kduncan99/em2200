//  JumpKeyKeyin.h
//
//  Intermediate base class for all jump-key keyins



#ifndef     EXECLIB_JUMP_KEY_KEYIN_H
#define     EXECLIB_JUMP_KEY_KEYIN_H



#include    "Exec.h"
#include	"KeyinActivity.h"



class   JumpKeyKeyin : public KeyinActivity
{
private:
    // KeyinThread interface (we don't implement these - subordinate classes must do so)
    void                        handler() = 0;
    bool                        isAllowed() const = 0;

protected:
    void                        displayJumpKeys() const;

public:
    JumpKeyKeyin( Exec* const                       pExec,
                  const SuperString&                KeyinId,
                  const SuperString&                Option,
                  const std::vector<SuperString>&   Params,
                  const Word36&                     Routing );
};



#endif

