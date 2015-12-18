//  Emitter.h
//
//  Interface and code for abstract class defining behavior for any object
//  which wishes to emit events to listeners.



#ifndef     MISCLIB_EMITTER_H
#define     MISCLIB_EMITTER_H



#include    "Event.h"
#include    "Listener.h"



class   Emitter
{
private:
    typedef std::list<Listener*>        LISTENERS;
    typedef LISTENERS::iterator         ITLISTENERS;
    typedef LISTENERS::const_iterator   CITLISTENERS;

    LISTENERS                           m_Listeners;

protected:
    void    notifyListeners( Event* const pEvent )
    {
        for ( CITLISTENERS it = m_Listeners.begin(); it != m_Listeners.end(); ++it )
            (*it)->listenerEventTriggered( pEvent );
    }

public:
    inline void         registerListener( Listener* const pListener )           { m_Listeners.push_back( pListener ); }
    inline void         registerPriorityListener( Listener* const pListener )   { m_Listeners.push_front( pListener ); }
    inline void         unregisterListener( Listener* const pListener )         { m_Listeners.remove( pListener ); }
};



#endif
