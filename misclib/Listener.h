//  Listener.h
//
//  A very generic interface for objects which wish to listen to some other object's noise.
//  See Emitter class



#ifndef     MISCLIB_LISTENER_H
#define     MISCLIB_LISTENER_H



#include    "Event.h"



class   Listener
{
protected:
    Listener(){}

public:
    virtual ~Listener(){};
    virtual void    listenerEventTriggered( Event* const pEvent ) = 0;
};



#endif
