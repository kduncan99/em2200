//  IntrinsicActivity - an activity which is an actual host OS thread, compiled into the library
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implements a basic state machine, which is the easiest way to handle intrinsic activities which
//  must constantly check for exception conditions so they can shut down or divert as quickly as possible.
//
//  Derived classes may dispense with state machine handling by simply implementing their own worker.



#ifndef EXECLIB_INTRINSIC_ACTIVITY_H
#define EXECLIB_INTRINSIC_ACTIVITY_H



#include    "Activity.h"



class IntrinsicActivity : public Activity 
{
public:
    typedef UINT32              STATE;

protected:
    typedef void                (*HANDLER)(IntrinsicActivity* const pObject);

    const static STATE          m_InitialState = 0;
    const static STATE          m_TerminalState = 0xFFFF;

private:
    class StateEntry
    {
    public:
        HANDLER                     m_Handler;
        STATE                       m_NextState;
        COUNT32                     m_DelayMSec;

        StateEntry()
            :m_Handler( 0 ),
            m_NextState( m_TerminalState ),
            m_DelayMSec( 0 )
        {}

        StateEntry( HANDLER         handler,
                    const STATE     nextState,
                    const COUNT32   delayMSec )
                    :m_Handler(handler),
                    m_NextState( nextState ),
                    m_DelayMSec( delayMSec )
        {}
    };

    typedef std::map<STATE, StateEntry>         STATEENTRIES;
    typedef STATEENTRIES::iterator              ITSTATEENTRIES;
    typedef STATEENTRIES::const_iterator        CITSTATEENTRIES;

    STATE                       m_CurrentState;
    COUNT32                     m_DelayMSec;
    STATE                       m_NextState;
    STATEENTRIES                m_StateEntries;

    const static COUNT32        m_DelayIncrement = 50;      // msec's

    //  Worker interface
    virtual void                worker();

protected:
    IntrinsicActivity( Exec* const          pExec,
                       const std::string&   activityName,
                       RunInfo* const       pRunInfo );
    virtual ~IntrinsicActivity();

    //  Activity interface
    virtual void                dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits ) = 0;

    inline void establishStateEntry( const STATE        triggerState,
                                        HANDLER         handler,
                                        const STATE     nextState,
                                        const COUNT32   delayMSec = 0 )
    {
        m_StateEntries[triggerState] = StateEntry( handler, nextState, delayMSec );
    }

public:
    inline STATE    getCurrentState() const                 { return m_CurrentState; }
    inline void     setDelay( const COUNT32 delayMSec )     { m_DelayMSec = delayMSec; }
    inline void     setNextState( const STATE nextState )   { m_NextState = nextState; }
};



#endif
